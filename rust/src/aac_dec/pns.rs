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
//! Perceptual noise substitution (PNS)
//!
//! See ISO 14496-3:2009, ch. 4.6.13

use super::channel_info::IcsInfo;
use super::constants;
use itertools::{izip, Itertools};

pub const NOISE_OFFSET: i16 = 90; // cf. ISO 14496-3:2009, ch. 4.6.13.3
const NO_OF_BANDS: usize = constants::MAX_WINS_X_SFBS;

/// Restricts the `noise_nrg_val` between [0 - 64 - NOISE_OFFSET; 255 + 64 + NOISE_OFFSET].
#[inline(always)]
pub fn clamp_noise_range(noise_nrg_val: i16) -> i16 {
    noise_nrg_val.clamp(0 - 64 - NOISE_OFFSET, 255 + 64 - NOISE_OFFSET)
}

// ---- Data ---------------------------------------//
/// Perceptual noise substitution inter-channel data.
/// Contains PNS related data of a channel pair.
#[derive(Copy, Clone, Debug)]
#[repr(C)]
pub struct PnsInterChannelData {
    is_correlated: [bool; NO_OF_BANDS],
    current_seed: i32,
    random_seed: [i32; NO_OF_BANDS],
}

/// Perceptual noise substitution data
/// Contains PNS data of a spectral channel
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
#[repr(C)]
pub struct PnsData {
    is_pns_used: [bool; NO_OF_BANDS],
    is_pns_active: bool,
}

// ---- Traits -----------------------------------//
impl Default for PnsData {
    fn default() -> Self {
        Self {
            is_pns_used: [false; NO_OF_BANDS],
            is_pns_active: Default::default(),
        }
    }
}

impl Default for PnsInterChannelData {
    fn default() -> Self {
        Self {
            is_correlated: [false; NO_OF_BANDS],
            current_seed: Default::default(),
            random_seed: [0; NO_OF_BANDS],
        }
    }
}

// ---- Methods for PnsData struct --------------//
impl PnsData {
    /// Creates as new `PnsData` instance.
    pub fn new() -> Self {
        Self::default()
    }

    /// Get a mutable reference to PnsData::is_pns_used[]
    pub fn is_pns_used_mut(&mut self) -> &mut [bool] {
        &mut self.is_pns_used
    }

    /// Get value of PnsData::is_pns_active
    pub fn is_pns_active(&self) -> bool {
        self.is_pns_active
    }

    /// Set PnsData::is_pns_active
    pub fn set_is_pns_active(&mut self, val: bool) {
        self.is_pns_active = val;
    }

    /// Re-init PNS data
    ///
    /// # Examples
    /// ```
    /// use aac::aac_dec::pns::*;
    ///
    /// let mut pns: PnsData = Default::default();
    /// pns.init();
    /// ```
    pub fn init(&mut self) {
        self.is_pns_active = false;
        self.is_pns_used[..].fill(false);
    }

    /// Apply PNS (i.e. generate noise) on the bands flagged as noisy bands within the ICS.
    /// # Parameters
    /// - `pns_interchannel_data`: Perceptual noise substitution inter-channel data.
    /// - `ics_info`: Individual channel stream info data.
    /// - `spectrum`: In/out buffer (slice) containing the spectrum data of the respective channel.
    /// - `scalefactors`: In buffer (slice) containing the spectral scale factors for each sfb in
    ///   each window.
    /// - `channel`: current channel.
    ///
    /// # Examples
    /// ```
    /// use aac::aac_dec::{channel_info::*, pns::*};
    ///
    /// let mut pns: PnsData = Default::default();
    /// let mut pns_ic: PnsInterChannelData = Default::default();
    /// let ics_info: IcsInfo = Default::default();
    /// let mut spec = vec![0.0_f32; 32];
    /// let scalefactors = vec![1_i16; 32];
    /// let ch = 0;
    ///
    /// pns.apply(&mut pns_ic, &ics_info, &mut spec, &scalefactors, ch);
    /// ```
    pub fn apply(
        &self,
        pns_interchannel_data: &mut PnsInterChannelData,
        ics_info: &IcsInfo, // individual_channel_stream_info
        spectrum: &mut [f32],
        scalefactors: &[i16],
        channel: usize,
    ) {
        if self.is_pns_active {
            let band_offsets = ics_info.scale_factor_bands();
            let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();

            let mut spec_window =
                spectrum.chunks_exact_mut(spectrum.len() / ics_info.windows_per_frame());
            let mut random_seed_window = pns_interchannel_data
                .random_seed
                .chunks_exact_mut(bands_per_window);

            // take a set of scalefactors, which will be applied over all windows of a group
            for ((group, scf), is_pns_used, is_correlated) in izip!(
                scalefactors.chunks_exact(bands_per_window).enumerate(),
                self.is_pns_used.chunks_exact(bands_per_window),
                pns_interchannel_data
                    .is_correlated
                    .chunks_exact(bands_per_window),
            )
            .take(ics_info.n_window_groups())
            {
                // iterate over all windows within a group
                // the same scalefactors must be applied over all windows
                let win_per_group = usize::from(ics_info.window_group_length(group));
                for _win in 0..win_per_group {
                    let spec = spec_window.next().unwrap();
                    let random_seed = random_seed_window.next().unwrap();

                    // iterate over all scale factor bands.
                    for (
                        is_pns_used,
                        is_correlated,
                        random_seed,
                        (band_offset_curr, band_offset_next),
                        scf_band,
                    ) in izip!(
                        is_pns_used.iter(),
                        is_correlated.iter(),
                        random_seed.iter_mut(),
                        band_offsets.iter().tuple_windows(),
                        scf.iter()
                    )
                    .take(ics_info.max_sf_bands())
                    {
                        if *is_pns_used {
                            let random_state;

                            if channel > 0 && *is_correlated {
                                random_state = random_seed;
                            } else {
                                *random_seed = pns_interchannel_data.current_seed;
                                random_state = &mut pns_interchannel_data.current_seed;
                            }

                            let spec_start = usize::from(*band_offset_curr);
                            let spec_end = usize::from(*band_offset_next);

                            self.generate_random_vector_and_scale(
                                &mut spec[spec_start..spec_end],
                                *scf_band,
                                random_state,
                            );
                        }
                    }
                }
            }
        }
    }

    /// Generate a vector of noise of given length. The noise values are
    /// scaled in order to yield a noise energy of 1.0
    /// (updates spec with exponent of generated noise vector.)
    ///
    /// #  Parameters:
    /// - `spectrum_buff`:  Out buffer (slice), where the noise values will be written to.
    /// - `scalefactor`:  In buffer (slice) containing the spectral scale factors for each sfb in
    ///   each window.
    /// - `random_state`:  Reference to the state of the random generator being used.
    fn generate_random_vector_and_scale(
        &self,
        spectrum_buff: &mut [f32],
        scalefactor: i16,
        random_state: &mut i32,
    ) {
        let table = [
            0.5_f32,
            0.59460355750136050_f32,
            0.70710678118654760_f32,
            0.84089641525371450_f32,
        ];
        let sf_exponent = (scalefactor >> 2) + 1;
        let sf_mantissa =
            table[(scalefactor & 0x03) as usize] * 2.0_f32.powi(i32::from(sf_exponent));

        let mut rs = *random_state;
        let mut nrg = 0.0_f32;

        for spec in spectrum_buff.iter_mut() {
            // The overflow here is intentional.
            rs = rs.wrapping_mul(1664525_i32).wrapping_add(1013904223_i32);
            *spec = rs as f32;
            nrg += *spec * *spec;
        }

        // Store random state value
        *random_state = rs;

        let scale = nrg.sqrt().recip() * sf_mantissa;

        for spec in spectrum_buff.iter_mut() {
            *spec *= scale;
        }
    }
}

// ---- Methods for PnsInterChannelData struct --//
impl PnsInterChannelData {
    /// Re-init PNS InterChannel data
    /// # Examples
    /// ```
    /// use aac::aac_dec::pns::*;
    ///
    /// let mut pns_ic: PnsInterChannelData = Default::default();
    /// pns_ic.init();
    /// ```
    pub fn init(&mut self) {
        self.is_correlated[..].fill(false);
    }

    /// Map midside info (`ms_used[]`) to noise correlation info of PNS (`self.is_correlated[]`).
    /// `ms_used[]` is updated on return to reset stereo tools signalling.
    ///
    /// # Parameters
    /// - `ics_info`:  Individual channel stream info data.
    /// - `pnsdata_left`: PnsData of left channel of a channel pair.
    /// - `pnsdata_right`: PnsData of right channel of a channel pair.
    /// - `ms_used`: In/Out buffer (slice) which indicates whether the same random vector is used
    ///   for both channels of a channel pair.
    /// # Examples
    /// ```
    /// use aac::aac_dec::{channel_info::*, pns::*};
    ///
    /// let scf_bands = 50;
    /// let mut ms_used_buff = vec![0_u8; scf_bands]; // will store the output
    /// let mut ics_info: IcsInfo = Default::default();
    /// let mut pns_ic: PnsInterChannelData = Default::default();
    /// let mut pns: Vec<PnsData> = Vec::new();
    /// pns.push(Default::default()); // L
    /// pns.push(Default::default()); // R
    ///
    /// pns_ic.map_midside_mask_to_pns_correlation(&ics_info, &pns[0], &pns[1], &mut ms_used_buff);
    /// ```
    pub fn map_midside_mask_to_pns_correlation(
        &mut self,
        ics_info: &IcsInfo,
        pnsdata_left: &PnsData,
        pnsdata_right: &PnsData,
        ms_used: &mut [u8], // buffer is part of JointStereoData.
    ) {
        let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();

        for ((group, is_correlated), is_pns_used_left, is_pns_used_right) in izip!(
            self.is_correlated
                .chunks_exact_mut(bands_per_window)
                .enumerate(),
            pnsdata_left.is_pns_used.chunks_exact(bands_per_window),
            pnsdata_right.is_pns_used.chunks_exact(bands_per_window),
        )
        .take(ics_info.n_window_groups())
        {
            let group_mask = u8::try_from(1 << group).unwrap();
            // iterate up to max_sf_bands() or up to last element of ms_used[] (depending on which
            // one is smaller)
            for (ms_item, is_correlated_band, is_pns_used_band_left, is_pns_used_band_right) in
                izip!(
                    ms_used.iter_mut(),
                    is_correlated.iter_mut(),
                    is_pns_used_left.iter(),
                    is_pns_used_right.iter(),
                )
                .take(ics_info.max_sf_bands())
            {
                if *ms_item & group_mask != 0 {
                    *is_correlated_band = true;

                    if *is_pns_used_band_left && *is_pns_used_band_right {
                        *ms_item ^= group_mask;
                    }
                }
            }
        }
    }
}

// ---------- Unit tests ---------
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_pns_data_generate_random_vector_and_scale() {
        const BUFF_LEN: usize = 8;
        let c_ref_random_vector_and_scale_data: [f32; BUFF_LEN] = [
            9.9541836_f32,
            11.7462187_f32,
            -7.6096473_f32,
            -14.0049238_f32,
            16.1952362_f32,
            -15.9470921_f32,
            14.4937534_f32,
            -15.1784744_f32,
        ];
        const CMP_THRESH: f32 = 0.000001_f32;

        let pns: PnsData = Default::default();
        let mut spec = vec![0.0_f32; BUFF_LEN];
        let scf = 21;
        let mut random_state = 0;

        // ------- Execute DUT -------
        pns.generate_random_vector_and_scale(&mut spec, scf, &mut random_state);

        // ------- Test DUT -------
        for (s, c_ref) in izip!(spec.iter_mut(), c_ref_random_vector_and_scale_data.iter()) {
            assert!(((*s).abs() - (*c_ref).abs()) < CMP_THRESH);
        }
    }

    #[test]
    fn test_pns_data_is_active() {
        let mut pns: PnsData = Default::default();

        // set PnsData::is_pns_active
        pns.set_is_pns_active(true);

        // get PnsData::is_pns_active
        assert!(pns.is_pns_active());
    }

    #[test]
    fn test_pns_data_is_pns_used() {
        let mut pns: PnsData = Default::default();

        // set PnsData::is_pns_used[]
        let set_is_pns_used = pns.is_pns_used_mut();
        for item in set_is_pns_used.iter_mut() {
            *item = true;
        }

        // Get PnsData::is_pns_used[]
        for item in pns.is_pns_used.iter() {
            assert!(*item);
        }
    }

    #[test]
    fn test_pns_data_apply() {
        // --------------- Create ICSInfo ---------------
        let mut ics_info = IcsInfo::new();
        // init with hardcoded ics_info data.
        ics_info_unit_test_demo::init(&mut ics_info);

        // --------------- Create PnsData ---------------
        let mut pns: PnsData = Default::default();
        let mut pns_ic: PnsInterChannelData = Default::default();

        // --------------- Create input data ---------------
        const NUM_SAMPLES: u16 = 1024;
        let mut spec = vec![0.0_f32; usize::from(NUM_SAMPLES)];

        let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
        // set of scalefactors for each group.
        let mut scalefactors = vec![0_i16; ics_info.n_window_groups() * bands_per_window];
        for (val, scf_per_group) in scalefactors
            .chunks_exact_mut(bands_per_window)
            .enumerate()
            .take(ics_info.n_window_groups())
        {
            scf_per_group.fill(std::convert::TryInto::try_into(val).unwrap());
        }

        pns.is_pns_active = true;
        pns.is_pns_used.fill(true);

        // ------- Execute DUT -------
        pns.apply(&mut pns_ic, &ics_info, &mut spec, &scalefactors, 0);

        // --------------- Test PnsData ---------------
        // -- Compare results against output obtained from C ref code --
        const CMP_THRESH: f32 = 0.000001_f32;
        const C_REF_TABLE: [f32; 1024] = [
            0.4491784, 0.5300432, -0.3433822, -0.6319664, 0.5235039, -0.5154827, 0.4685042,
            -0.4906375, -0.6778039, -0.5758770, -0.2799149, -0.3613798, -0.6122463, -0.1821402,
            0.0107004, -0.7693276, -0.4326750, 0.4614069, 0.5685046, -0.5260215, -0.5069729,
            -0.4889938, 0.4271276, 0.2637282, 0.0132492, 0.2832157, -0.0175411, -0.4137373,
            0.5192699, 0.1531039, 0.3932068, 0.0253870, 0.4250000, 0.4318883, -0.0100727,
            -0.4294274, -0.4928137, 0.2175190, -0.0295345, -0.5491799, 0.2842760, 0.1077434,
            -0.5107062, -0.2326090, 0.0841605, 0.1682915, 0.0974434, 0.4420750, 0.3011472,
            -0.4935632, 0.1415618, 0.1630995, -0.1017035, 0.4597385, 0.0397004, -0.3942776,
            0.3615731, 0.4164306, 0.0345581, 0.4196930, 0.1404479, -0.1302436, 0.1275945,
            0.3934835, 0.4056050, 0.2375599, 0.2456584, -0.1715144, 0.2771088, 0.1645195,
            -0.2713507, -0.1529681, 0.3950430, 0.3709872, 0.5795108, -0.0961917, -0.0613305,
            0.3597167, -0.1309151, -0.1002060, -0.1002363, 0.3976953, 0.3291867, 0.3372402,
            0.0252385, -0.3010534, 0.1480961, -0.3189250, 0.1358117, -0.3200583, 0.2689109,
            -0.0818852, 0.2859630, -0.2897983, -0.1599941, 0.0593638, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, -0.5874832,
            0.2041220, -0.5092610, -0.5948538, 0.6903418, -0.3494114, -0.6087247, 0.1754823,
            -0.4030705, 0.9116043, -0.0550174, -0.0590305, 0.1442121, -0.7544521, -0.2625013,
            -0.5840358, -0.5392148, 0.2205669, -0.5789809, -0.5704197, 0.5468314, -0.4332039,
            0.3275986, 0.2117494, -0.4581472, -0.2254032, 0.1686948, -0.2683056, 0.4030561,
            -0.3188651, -0.3709453, 0.5496843, 0.3722130, 0.2067519, 0.2386906, 0.2405317,
            -0.4370094, -0.4872805, -0.1220671, 0.1685308, -0.0840623, 0.2867413, -0.4677424,
            0.4692627, -0.3409925, 0.2524753, -0.0043120, 0.1614687, 0.2747758, 0.3559764,
            0.2459351, -0.2150962, -0.3946613, 0.3693927, 0.2363562, -0.3699264, -0.4518511,
            -0.1449773, -0.4260632, -0.2680133, -0.0283661, 0.0310773, 0.2799613, 0.0997837,
            0.3602965, 0.4039990, -0.1867267, 0.3216364, -0.1453947, 0.0979471, -0.2112443,
            0.1927293, 0.2149291, -0.1176464, -0.4237102, 0.4011069, -0.4101547, -0.4531489,
            -0.1544344, 0.2993743, -0.2448712, 0.0161840, -0.2524864, 0.2729042, -0.0207711,
            -0.1112781, -0.0585456, 0.4347144, -0.2783713, -0.2082154, -0.3285288, -0.2154262,
            -0.4083081, 0.1863901, 0.2904001, 0.1880391, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, -0.0114328, -0.0820903,
            0.6791732, 0.7292833, -0.7466339, 0.5640722, 0.0309747, -0.3512845, -0.6890149,
            -0.2008439, 0.2641473, -0.6443186, 0.3308544, 0.4772110, 0.7147474, -0.3897965,
            0.3880069, -0.3028021, 0.5671196, -0.6604066, 0.2634394, 0.3152716, -0.4872729,
            -0.2448600, 0.4082443, 0.2087447, 0.3685572, -0.4332899, -0.0463044, -0.4904900,
            -0.5213971, -0.1493085, 0.2579217, -0.2932896, -0.4641573, 0.3084526, -0.0666823,
            0.5024012, 0.0259108, -0.3755967, -0.1770279, 0.4256832, -0.6173909, 0.0876685,
            0.5477328, 0.1261152, -0.0138716, -0.3525198, 0.1572218, 0.5588840, 0.2528707,
            -0.1168637, -0.1735293, 0.0371668, 0.2031249, -0.2686951, -0.0559671, 0.1107052,
            0.4337615, -0.4732152, -0.4735441, 0.1839676, -0.0954427, 0.0108666, -0.0987375,
            -0.2474252, 0.1038905, 0.4727088, 0.3852147, 0.3382140, -0.1788004, 0.2528835,
            -0.3580935, -0.3066720, -0.2419504, -0.2426469, -0.3632242, 0.0329947, -0.2921092,
            0.2885424, 0.2001678, -0.2639109, 0.3396413, -0.0664552, 0.3559314, -0.1769882,
            0.2328333, 0.0556793, 0.1526598, 0.0146729, -0.3233805, 0.3314176, 0.1969879,
            -0.2006029, -0.3905176, 0.2927546, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, -0.9188604, -0.1545272, 0.4850716,
            -0.5574369, 0.9753891, -0.4365437, 0.2623525, -0.4510324, 0.7254599, -0.1036760,
            -0.9349824, -0.0545976, -0.7710404, -0.0915870, -0.1056637, 0.8945152, 0.9512948,
            0.2161506, -0.5663034, 0.3766046, -0.5130191, 0.4228013, 0.2818601, -0.5348047,
            0.0868089, -0.1666728, 0.5922347, -0.4698354, 0.5036919, -0.3047460, -0.5375926,
            -0.5617859, -0.2798883, 0.1250260, -0.5731009, 0.2015317, -0.2866209, -0.5689881,
            0.1650450, 0.6012753, -0.1455016, -0.6392844, -0.1990990, -0.3873592, -0.1108104,
            0.5143114, 0.4073014, -0.2403732, 0.0130794, 0.3749453, 0.3737289, 0.3575023,
            -0.1726388, -0.2937419, -0.1871299, -0.5953169, -0.2205805, 0.0367314, -0.5565470,
            0.1972844, 0.0176988, 0.3859054, 0.4944156, -0.1685656, 0.5277206, 0.1106320,
            0.1006136, -0.5409196, 0.4230218, 0.3830951, -0.0849425, 0.0466859, 0.4314734,
            0.3296395, 0.2847051, -0.4538679, 0.3753675, -0.3522973, 0.4816628, 0.0145550,
            0.3496204, -0.1472664, 0.4268816, 0.0492378, -0.0825311, 0.2564554, -0.2756960,
            -0.4165274, -0.0277412, 0.3180858, 0.1800821, -0.3510245, 0.4018983, 0.2615182,
            -0.4035197, -0.3364778, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.8681597, -0.6147645, 0.5311484, 0.0214053,
            -0.8829047, 0.6255880, 0.1131909, 0.4801252, 0.5029560, -0.5492477, 0.6170384,
            -0.6919822, 0.7117461, -0.5759813, 0.4015812, 0.6439015, -0.9113546, 0.3935652,
            0.3171653, 0.5728514, 0.5005733, 0.4766929, -0.1029945, -0.6956089, 0.3956162,
            0.1854629, -0.4634390, 0.1903673, -0.4278985, 0.3274794, -0.4382470, -0.1853406,
            -0.4712932, -0.6373912, -0.0247059, -0.5181379, 0.6783967, -0.3721374, -0.2962863,
            -0.1643725, 0.4340414, 0.2574624, -0.3868519, 0.5443970, 0.2115455, -0.0405822,
            -0.4414823, 0.3570679, 0.3154485, -0.2074665, -0.4838062, -0.1396423, -0.1781539,
            -0.5198907, 0.4385800, -0.3935922, -0.1959397, 0.1103116, 0.1290825, 0.4581345,
            0.1653218, 0.4334326, -0.4723108, 0.4429826, 0.2845909, 0.3519553, 0.3076241,
            -0.4506657, -0.4757732, 0.1521362, 0.0197335, 0.1128571, 0.2551177, 0.3015144,
            -0.5721241, 0.4713484, -0.0922418, 0.0985645, -0.4304903, -0.4924886, 0.1121341,
            0.1451779, 0.4897528, -0.1988777, 0.0772105, -0.1924137, 0.5106198, -0.4425708,
            -0.1783804, 0.5579813, -0.4229333, -0.1045670, -0.1065398, -0.2025160, 0.0470116,
            0.1184213, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, -0.9888708, -0.5228137, -0.3242709, 0.2405452, -0.8976364, 0.2367895,
            0.4225712, 0.6114137, 0.4458146, 0.8505767, 0.3501061, 0.6077894, -0.5589986,
            -0.0218187, -0.7457383, 0.7383309, 0.6565171, 0.7830881, -0.5388138, 0.2822256,
            -0.6007766, 0.3145218, 0.5507103, 0.3821293, -0.0469267, -0.1252710, 0.1781461,
            0.6748489, 0.4591572, 0.0689318, 0.3448184, 0.4366245, -0.4895388, 0.4990298,
            -0.4192514, -0.4739673, -0.6115836, -0.3945554, -0.5225133, 0.5374864, -0.1488708,
            -0.0358289, -0.3952729, -0.3780297, 0.1590936, 0.6458406, -0.0638010, 0.1342041,
            0.2453888, 0.1372972, -0.2864081, 0.0490046, 0.5280814, -0.2236802, -0.5225219,
            -0.4292811, 0.0408470, -0.4522492, 0.3574803, 0.2691866, -0.3629666, -0.4704387,
            0.4667333, -0.3827786, -0.0872468, -0.2347666, -0.0116150, 0.4769520, 0.1197209,
            -0.1038303, -0.0960224, -0.6122878, -0.1727030, 0.4815373, 0.1573156, -0.6075428,
            0.0959077, 0.0274180, 0.1088790, 0.5723786, 0.4088582, 0.3813722, -0.5211804,
            -0.3792545, -0.0921339, 0.3190723, -0.2421965, -0.4555315, 0.1637340, -0.1510862,
            0.1791720, -0.3198098, 0.1742386, 0.3022892, -0.0590728, 0.0207958, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.2035413,
            0.7959991, -0.9152800, 0.6980106, -0.8746083, 0.3354995, -1.0441496, 0.1795874,
            0.3615275, -0.6422454, 0.9348627, 0.7634465, 0.4919191, -0.7095194, -0.8522198,
            -0.7268556, -0.9332032, 0.7873407, 0.2650237, 0.6625625, -0.0570130, 0.4760127,
            -0.4407377, -0.4619354, -0.8563440, -0.1811417, 0.2273829, 0.7380291, 0.0264670,
            0.1412698, 0.2932702, -0.5897071, 0.5223362, 0.7467070, -0.3159695, -0.7844348,
            -0.2504936, -0.4306849, -0.2079287, -0.2739666, 0.0390495, 0.9232370, 0.6371753,
            0.6112185, 0.4847679, -0.4510340, -0.3662862, 0.2698636, 0.1230706, 0.4649405,
            0.1997871, 0.4713567, 0.3822617, 0.4756771, 0.4713590, -0.5163301, 0.3482581,
            0.3049947, -0.5385358, 0.3293799, -0.3565402, 0.6992724, -0.2709380, 0.4301485,
            -0.4713836, -0.4886886, -0.0798278, -0.2127282, 0.0946970, -0.5526738, 0.5278208,
            0.0528580, -0.0397542, -0.6428480, -0.6215304, 0.3232357, 0.6157996, 0.2090685,
            -0.1237666, -0.2456485, 0.1939254, 0.4991692, -0.3840200, 0.3377503, -0.3043633,
            -0.0144953, -0.2220396, 0.3877449, -0.2739004, -0.4273235, 0.2518003, 0.2650819,
            0.5109833, -0.4849313, -0.3817732, -0.3547812, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, -0.2510008, 1.3161554,
            0.1644867, 0.4215181, -0.4795674, -0.9501911, 0.9275683, 0.0822733, -0.4457936,
            1.0058689, -0.4609691, 0.7596074, 0.3791344, -0.6937881, -0.8639386, 0.7927958,
            -0.7464858, -0.6979536, -0.2948076, -0.9320453, -0.1806324, 0.4380602, -0.1322887,
            0.2076743, 0.7307501, 0.9517115, 0.2845213, -0.4406164, -0.3321156, 0.3238906,
            -0.7255694, 0.4091019, -0.5445849, -0.6965461, 0.5557550, -0.0191320, -0.5626655,
            -0.2815359, 0.0039893, -0.5644412, -0.6814864, -0.1442097, 0.6267958, -0.6383120,
            0.6325498, -0.0054095, -0.4552979, -0.3389108, 0.4883775, 0.2211159, -0.4067824,
            -0.0770550, -0.3775833, -0.5887621, 0.5695428, -0.0727444, -0.5205220, -0.3619253,
            -0.5451977, -0.3175861, -0.3073561, 0.1040583, -0.5442173, 0.5285591, 0.0800992,
            -0.3169224, -0.3748485, 0.5213018, 0.1536750, -0.6400652, -0.2001665, -0.0853624,
            -0.6698903, 0.4139841, -0.3135469, 0.6645001, 0.3774004, 0.0204482, -0.3239157,
            0.3340187, -0.4583759, -0.2629091, 0.2438252, -0.0891465, -0.0510397, -0.3085643,
            0.4851327, 0.4523119, -0.0276774, 0.5250426, 0.4782820, -0.4329805, -0.0257074,
            0.4730213, -0.1606656, 0.4156085, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000, 0.0000000,
            0.0000000, 0.0000000, 0.0000000, 0.0000000,
        ];

        for (s, c_ref) in izip!(spec.iter_mut(), C_REF_TABLE.iter()) {
            assert!(((*s).abs() - (*c_ref).abs()) < CMP_THRESH);
        }
    }

    // The following test evaluates that the output of ms_used[] and
    // PnsInterChannelData::is_correlated() are as expected.
    // Description of the ms_used[] flags:
    //      8 bit:  gr7 gr6 gr5 gr4 gr3 gr2 gr1 gr0  (group of windows)
    //      0xFF:     1   1   1   1   1   1   1   1  (flag for a group of windows. Maximum 8
    // windows/group)

    // --------------- Test #1: ---------------
    // input:
    //     - ms_used[] = 0xFF (1 1 1 1 1 1 1 1)
    //     - is_pns_used_L == is_pns_used_R == true
    // expected output:
    //     - PnsInterChannelData::is_correlated[g][b] == true
    //     - ms_used[] = 0xF8 (1 1 1 1 1 0 0 0)
    #[test]
    fn test_map_midside_mask_to_pns_correlation() {
        // --------------- Create ICSInfo ---------------
        let mut ics_info = IcsInfo::new();
        // init with hardcoded ics_info data.
        ics_info_unit_test_demo::init(&mut ics_info);

        // ------- Create PnsData for 2 channels -------
        let mut pns: Vec<PnsData> = vec![
            PnsData {
                is_pns_used: [false; NO_OF_BANDS],
                is_pns_active: false
            };
            2
        ];
        for pns_item in pns.iter_mut() {
            pns_item.is_pns_active = true;
            pns_item.is_pns_used.fill(true);
        }

        // -------- Create PnsInterChannelData ----------
        let mut ic_data: PnsInterChannelData = Default::default();

        // -------- Create input data (ms_used[]) -------
        let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
        let mut ms_buff = vec![255_u8; bands_per_window];

        // ------- Execute DUT -------
        ic_data.map_midside_mask_to_pns_correlation(&ics_info, &pns[0], &pns[1], &mut ms_buff);

        // ------- Test DUT -------
        // test expected output: is_corr[g][band] == true
        for is_corr_per_group in ic_data
            .is_correlated
            .chunks_exact_mut(bands_per_window)
            .take(ics_info.n_window_groups())
        {
            for is_corr_band in is_corr_per_group.iter_mut().take(ics_info.max_sf_bands()) {
                assert!(*is_corr_band);
            }
        }

        // test expected output: ms_used[] == 0xF8
        // ms_buff[]: The flags correspondent to the number of window groups
        // (ics_info.n_windows_group) are cleared: 8 bit:  gr7 gr6 gr5 gr4 gr3 gr2 gr1 gr0
        // 0xF8:     1   1   1   1   1   0   0   0
        for is_ms_used in ms_buff.iter_mut().take(ics_info.max_sf_bands()) {
            assert!(*is_ms_used == 0xF8);
        }
    }
}

#[cfg(test)]
// This module acts as a helper, for initializing an instance of IcsInfo with particular data.
mod ics_info_unit_test_demo {
    use crate::{
        aac_dec::{channel_info::IcsInfo, sr_info::SamplingRateInfo},
        common::{
            bitstream::{Bitstream, Mode},
            flags::ACFlags,
        },
    };

    // This function acts as a helper, for initializing an instance of IcsInfo with particular data.
    // IcsInfo instances are used in multiple places, which makes their initialization quite
    // tideous.
    pub fn init(ics_info: &mut IcsInfo) {
        const BS_BUFF_LEN: usize = 8;
        const NUM_SAMPLES: u16 = 1024;

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(NUM_SAMPLES, 3, 48000);

        // Byte blob meaning:
        // 0b0      : reserved
        // 0b10     : BlockType::Short
        // 0b0      : SineWindow
        // 0b1100   : max_sf_bands = 12
        // 0b1101101: scale factor grouping: 3|3|2
        let mut bitstream_writer = Bitstream::new(BS_BUFF_LEN, Mode::Writer);
        bitstream_writer.write(0b010011001101101, 15);
        bitstream_writer.sync();

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 15);

        // let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());
    }
}
