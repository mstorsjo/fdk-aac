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
//! Ducker (energy adjuster).
//!
//! Postprocessing for the decorrelated signals, i.e. adjusting of the signal enery (ducker),
//! to avoid audible reverberation tails after transients.

use super::super::hybrid::NUM_HYBRID_DATA_BANDS;
use super::{decorr_common::DecorrMaxParameterBands, decorr_constants::*, decorr_tables::*};
use itertools::izip;
use num_complex::Complex;

const PS_DUCK_PEAK_DECAY_FACTOR: f32 = 0.765928338364649;
const PS_DUCK_FILTER_COEFF: f32 = 0.25;

const DUCK_ALPHA: f32 = 0.8;
const DUCK_GAMMA: f32 = 1.5;

/// Ducker types.
#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
#[repr(C)]
pub(super) enum DuckerType {
    #[default] // See DECOR_MAX_PARAMETER_BANDS.
    Mps, // Force ducker type to MPS.
    Ps, // Force ducker type to PS.
}

#[derive(Debug, Clone)]
#[repr(C)]
pub(super) struct DuckerInstance {
    pub(super) ducker_type: DuckerType,

    hybrid_bands: usize,
    parameter_bands: DecorrMaxParameterBands,

    qs_next: Option<&'static [usize]>,
    map_proc_bands_2_hyb_bands: Option<&'static [usize]>,
    map_hyb_bands_2_proc_bands: Option<&'static [usize]>,

    smooth_dir_rev_nrg: [f32; 2 * DECORR_MAX_PARAMETER_BANDS],

    //  parametric stereo
    peak_decay: [f32; DECORR_MAX_PARAMETER_BANDS],
    peak_diff: [f32; DECORR_MAX_PARAMETER_BANDS],
}

impl DuckerInstance {
    pub(super) fn apply(
        &mut self,
        direct_nrg: &[f32; DECORR_MAX_PARAMETER_BANDS],
        cplx_out: &mut [Complex<f32>; NUM_HYBRID_DATA_BANDS],
        start_hyb_band: usize,
    ) {
        match self.ducker_type {
            DuckerType::Ps => {
                debug_assert!(self.parameter_bands == DecorrMaxParameterBands::Ps);

                let mut qs = start_hyb_band;
                let qs_next = self.qs_next.unwrap();
                let map_proc_bands_2_hyb_bands = self.map_proc_bands_2_hyb_bands.unwrap();
                let start_param_band = self.map_hyb_bands_2_proc_bands.unwrap()[start_hyb_band];
                let start_minus1 = start_param_band.saturating_sub(1);

                for (dir_nrg, proc_2_hyb_bands, qsnext, pk_decay, pk_diff, dir_rvb_nrg) in izip!(
                    direct_nrg[start_minus1..].iter(),
                    map_proc_bands_2_hyb_bands[start_minus1..].iter(),
                    qs_next[start_minus1..].iter(),
                    self.peak_decay[start_minus1..].iter_mut(),
                    self.peak_diff[start_minus1..].iter_mut(),
                    self.smooth_dir_rev_nrg[start_minus1..].iter_mut(),
                )
                .take(usize::from(self.parameter_bands) - start_minus1)
                {
                    *pk_decay = (*dir_nrg).max(*pk_decay * PS_DUCK_PEAK_DECAY_FACTOR);
                    *pk_diff += PS_DUCK_FILTER_COEFF * (*pk_decay - *dir_nrg - *pk_diff);
                    *dir_rvb_nrg = (*dir_rvb_nrg
                        + (PS_DUCK_FILTER_COEFF * (*dir_nrg - *dir_rvb_nrg)))
                        .max(0.0);

                    if *pk_diff == 0.0 && *dir_rvb_nrg == 0.0 {
                        qs = qs.max(*proc_2_hyb_bands);
                        let qs_next = (*qsnext).min(self.hybrid_bands);

                        while qs < qs_next {
                            cplx_out[qs] = Complex { re: 0.0, im: 0.0 };
                            qs += 1;
                        }
                    } else if *pk_diff != 0.0 {
                        let multiplication = 1.5_f32 * *pk_diff;
                        if multiplication > *dir_rvb_nrg {
                            qs = qs.max(*proc_2_hyb_bands);
                            let qs_next = (*qsnext).min(self.hybrid_bands);
                            let duck_gain = *dir_rvb_nrg / multiplication;

                            while qs < qs_next {
                                cplx_out[qs] *= duck_gain;
                                qs += 1;
                            }
                        }
                    }
                }
            }
            _ => {
                let mut qs = start_hyb_band;
                let mut qs_next;
                let qs_next_arr = self.qs_next.unwrap();

                let mut duck_gain;
                let start_param_band = self.map_hyb_bands_2_proc_bands.unwrap()[start_hyb_band];

                let reverb_nrg = self.calc_energy(cplx_out, start_hyb_band);

                for pb in start_param_band..usize::from(self.parameter_bands) {
                    let mut tmp1 = self.smooth_dir_rev_nrg[2 * pb] * DUCK_ALPHA
                        + direct_nrg[pb] * (1.0 - DUCK_ALPHA);
                    let mut tmp2 = self.smooth_dir_rev_nrg[2 * pb + 1] * DUCK_ALPHA
                        + reverb_nrg[pb] * (1.0 - DUCK_ALPHA);

                    self.smooth_dir_rev_nrg[2 * pb] = tmp1;
                    self.smooth_dir_rev_nrg[2 * pb + 1] = tmp2;

                    tmp1 *= DUCK_GAMMA;
                    qs_next = (qs_next_arr[pb]).min(self.hybrid_bands);

                    // True for about 20%.
                    if tmp2 > tmp1 {
                        // Gain smaller than 1.0.
                        duck_gain = (tmp1 / tmp2).sqrt();
                    }
                    // True for about 80%.
                    else {
                        tmp2 = self.smooth_dir_rev_nrg[2 * pb];
                        tmp1 = self.smooth_dir_rev_nrg[2 * pb + 1] * DUCK_GAMMA;
                        // True for about 20%.
                        if tmp2 > tmp1 {
                            if tmp1 <= tmp2 * 0.25 {
                                // Limit gain to 2.0.
                                if qs < self.hybrid_bands {
                                    while qs < qs_next {
                                        cplx_out[qs] *= 2.0;
                                        qs += 1;
                                    }
                                }
                                // Skip general gain*output section.
                                continue;
                            } else {
                                // Gain from 1.0 to 2.0.
                                duck_gain = (tmp2 / tmp1).sqrt();
                            }
                        }
                        // True for about 60%.
                        else {
                            // Gain = 1.0. Output does not change. Update `qs` index.
                            qs = qs_next;
                            continue;
                        }
                    }

                    while qs < qs_next {
                        cplx_out[qs] *= duck_gain;
                        qs += 1;
                    }
                }
            }
        }
    }

    #[inline(always)]
    pub(super) fn calc_energy(
        &mut self,
        inp_cplx: &[Complex<f32>; NUM_HYBRID_DATA_BANDS],
        start_hyb_band: usize,
    ) -> [f32; DECORR_MAX_PARAMETER_BANDS] {
        let map_hyb_bands_2_proc_bands = self.map_hyb_bands_2_proc_bands.unwrap();
        let mut energy = [0.0f32; DECORR_MAX_PARAMETER_BANDS];

        let start_band = start_hyb_band.saturating_sub(1);
        for (inp, pb) in izip!(
            inp_cplx[start_band..].iter(),
            map_hyb_bands_2_proc_bands[start_band..].iter()
        )
        .take(self.hybrid_bands - start_band)
        {
            energy[*pb] += (*inp).norm_sqr();
        }

        energy
    }

    pub(super) fn init(
        &mut self,
        hybrid_bands: usize,
        ducker_type: DuckerType,
        num_param_bands: DecorrMaxParameterBands,
        reset_nrg_buff: bool,
    ) -> i32 {
        let mut error_code = 0;

        match num_param_bands {
            DecorrMaxParameterBands::Ps => {
                debug_assert!(hybrid_bands == NUM_HYBRID_DATA_BANDS);
                self.map_hyb_bands_2_proc_bands = Some(&KERNELS_20_TO_71_PS);
                self.map_proc_bands_2_hyb_bands = Some(&KERNELS_20_TO_71_OFFSET_PS);
                self.parameter_bands = num_param_bands;
            }
            DecorrMaxParameterBands::Mps => {
                self.map_hyb_bands_2_proc_bands = Some(&KERNELS_28_TO_71);
                self.map_proc_bands_2_hyb_bands = Some(&KERNELS_28_TO_71_OFFSET);
                self.parameter_bands = num_param_bands;
            }
            DecorrMaxParameterBands::Ld => {
                debug_assert!(hybrid_bands == 64 || hybrid_bands == 32);
                self.map_hyb_bands_2_proc_bands = Some(&KERNELS_23_TO_64);
                self.map_proc_bands_2_hyb_bands = Some(&KERNELS_23_TO_64_OFFSET);
                self.parameter_bands = num_param_bands;
            }
            _ => {
                error_code = 1;
                return error_code;
            }
        }

        self.qs_next = Some(&self.map_proc_bands_2_hyb_bands.unwrap()[1..]);
        self.hybrid_bands = hybrid_bands;
        self.ducker_type = ducker_type;

        if reset_nrg_buff && (self.ducker_type == DuckerType::Ps) {
            self.smooth_dir_rev_nrg[..usize::from(num_param_bands)].fill(0.0);
        }

        error_code
    }

    pub(super) fn new() -> DuckerInstance {
        DuckerInstance {
            hybrid_bands: 0,
            parameter_bands: DecorrMaxParameterBands::None,
            ducker_type: DuckerType::Mps,
            qs_next: None,
            map_proc_bands_2_hyb_bands: None,
            map_hyb_bands_2_proc_bands: None,
            smooth_dir_rev_nrg: [0.0; 2 * DECORR_MAX_PARAMETER_BANDS],
            peak_decay: [0.0; DECORR_MAX_PARAMETER_BANDS],
            peak_diff: [0.0; DECORR_MAX_PARAMETER_BANDS],
        }
    }
}

impl Default for DuckerInstance {
    fn default() -> Self {
        Self::new()
    }
}
