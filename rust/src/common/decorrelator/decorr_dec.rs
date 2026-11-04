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
//! Decorrelator

use super::super::hybrid::NUM_HYBRID_DATA_BANDS;
use super::{
    decorr_common::{DecorrMaxParameterBands, DecorrType, ReverbBandFiltType},
    decorr_constants::*,
    decorr_filter::*,
    decorr_tables::*,
    ducker::*,
};
use itertools::izip;
use num_complex::Complex;

#[derive(Debug, Clone)]
#[repr(C)]
pub struct DecorrDec {
    state_buffer: Vec<Complex<f32>>,
    delay_buffer: Vec<Complex<f32>>,

    filter: [DecorrFilterInstance; NUM_HYBRID_DATA_BANDS],
    rev_filter_type: Option<&'static [ReverbBandFiltType]>,
    rev_filter_order: Option<&'static [usize]>,

    rev_band_offset: Option<&'static [usize]>,
    rev_delay: Option<&'static [usize]>,
    rev_band_delay_buffer_index: [usize; NUM_DECORR_BANDS],

    state_buffer_offset: [usize; NUM_ALLPASS_LINKS],

    ducker: DuckerInstance,

    num_bins: usize,
}

impl DecorrDec {
    /// Applies decorrelator and ducker on hybrid input data.
    /// Modified hybrid data will be returned.
    /// # Parameters
    /// - `data_in`:        In (hybrid) data.
    /// - `data_out`:       Out (hybrid) data.
    /// - `start_hyb_band`: Hybrid band to start with decorrelation.
    ///
    /// # Return
    /// 0 on success.
    pub fn apply(
        &mut self,
        data_in: &[Complex<f32>; NUM_HYBRID_DATA_BANDS],
        data_out: &mut [Complex<f32>; NUM_HYBRID_DATA_BANDS],
        start_hyb_band: usize,
    ) -> i32 {
        let mut error = 0;

        let rev_band_offset = self.rev_band_offset.unwrap();
        let rev_filter_type = self.rev_filter_type.unwrap();
        let rev_filter_order = self.rev_filter_order.unwrap();
        let rev_delay = self.rev_delay.unwrap();

        let direct_nrg = self.ducker.calc_energy(data_in, start_hyb_band);

        // Complex-valued hybrid bands.
        let mut start;
        let mut stop = 0;
        for rb in 0..NUM_DECORR_BANDS {
            start = stop.max(start_hyb_band);
            stop = rev_band_offset[rb].min(self.num_bins);

            if start < stop {
                let off_delay_buff =
                    self.filter[start].offset_delay_buffer + self.rev_band_delay_buffer_index[rb];
                let off_state_buff = self.filter[start].offset_state_buffer;
                match rev_filter_type[rb] {
                    ReverbBandFiltType::Delay => {
                        DecorrFilterInstance::apply_pass(
                            &data_in[start..],
                            &mut data_out[start..],
                            &mut self.delay_buffer[off_delay_buff..],
                            stop - start,
                            rev_delay[rb],
                        );
                    }
                    ReverbBandFiltType::IndepCplxPs => {
                        DecorrFilterInstance::apply_cplx_ps(
                            &self.filter[start..],
                            &data_in[start..],
                            &mut data_out[start..],
                            &mut self.delay_buffer[off_delay_buff..],
                            &mut self.state_buffer[off_state_buff..],
                            stop - start,
                            rev_filter_order[rb],
                            rev_delay[rb],
                            &mut self.state_buffer_offset,
                        );
                    }
                    ReverbBandFiltType::CommonReal => {
                        DecorrFilterInstance::apply_real(
                            self.filter[start].numerator_real.unwrap(),
                            &data_in[start..],
                            &mut data_out[start..],
                            &mut self.delay_buffer[off_delay_buff..],
                            &mut self.state_buffer[off_state_buff..],
                            stop - start,
                            rev_filter_order[rb],
                            rev_delay[rb],
                        );
                    }
                    _ => {
                        error = 1;
                        return error;
                    }
                }
            }
        }

        for (rev_dly, rev_band_index) in izip!(
            rev_delay.iter(),
            self.rev_band_delay_buffer_index.iter_mut()
        )
        .take(NUM_DECORR_BANDS)
        {
            *rev_band_index += 1;
            if *rev_band_index >= *rev_dly {
                *rev_band_index = 0;
            }
        }

        self.ducker.apply(&direct_nrg, data_out, start_hyb_band);

        error
    }

    /// Clears the internal buffers of `self`.
    pub fn deinit(&mut self) {
        if self.state_buffer.capacity() != 0 {
            self.state_buffer.clear();
            self.state_buffer.shrink_to_fit();
        }
        if self.delay_buffer.capacity() != 0 {
            self.delay_buffer.clear();
            self.delay_buffer.shrink_to_fit();
        }
    }

    /// Initializes and configures the decorrelator instance.
    /// # Parameters
    /// - `num_hyb_bands`:  Number of (hybrid) bands.
    /// - `decorr_type`:    Decorrelator type to use.
    /// - `decorr_config`:  Depending on decorrType, values of 0,1,2 are allowed.
    /// - `reset_buffs`:    Indicates whether the internal struct buffers have to be cleared.
    ///
    /// # Return
    /// 0 on success.
    pub fn init(
        &mut self,
        num_hyb_bands: usize,
        decorr_type: DecorrType,
        decorr_config: usize,
        reset_buffs: bool,
    ) -> i32 {
        let error;
        let mut offset_state_buff = 0;
        let mut offset_delay_buff = 0;

        match decorr_type {
            DecorrType::Ps => {
                self.rev_band_offset = Some(&REV_BANDOFFSET_PS_HQ);
                self.rev_delay = Some(&REV_DELAY_PS_HQ);
                self.rev_filter_order = Some(&REV_FILTERORDER_PS);
                self.rev_filter_type = Some(&REV_FILTTYPE_PS);
                // Initialize ring buffer offsets for PS specific filter implementation.
                self.state_buffer_offset
                    .clone_from(&STATE_BUFFER_OFFSET_INIT);
                self.state_buffer
                    .resize(DECORR_STATE_BUF_LEN_PS_HQ, Complex { re: 0.0, im: 0.0 });
                self.delay_buffer
                    .resize(DECORR_DELAY_BUF_LEN_PS_HQ, Complex { re: 0.0, im: 0.0 });
            }
            DecorrType::Usac => {
                // Reverb band layout is inherited from MPS standard.
                self.rev_band_offset = Some(&REV_BANDOFFSET_MPS_HQ[decorr_config]);
                self.rev_delay = Some(&REV_DELAY_USAC);
                self.rev_filter_order = Some(&REV_FILTERORDER_USAC);
                // The filter types are inherited from MPS standard.
                self.rev_filter_type = Some(&REV_FILTTYPE_MPS);
                self.state_buffer
                    .resize(DECORR_STATE_BUF_LEN_USAC_1, Complex { re: 0.0, im: 0.0 });
                self.delay_buffer
                    .resize(DECORR_DELAY_BUF_LEN_USAC_1, Complex { re: 0.0, im: 0.0 });
            }
            DecorrType::Ld => {
                if decorr_config > 2 || !(num_hyb_bands == 64 || num_hyb_bands == 32) {
                    error = 1;
                    return error;
                }
                self.rev_band_offset = Some(&REV_BANDOFFSET_LD[decorr_config]);
                // The delays in each reverb band are inherited from MPS standard.
                self.rev_delay = Some(&REV_DELAY_MPS);
                // The filter orders are inherited from MPS standard.
                self.rev_filter_order = Some(&REV_FILTERORDER_MPS);
                self.rev_filter_type = Some(&REV_FILTTYPE_LD);
                self.state_buffer
                    .resize(DECORR_STATE_BUF_LEN_MPS_LD_1, Complex { re: 0.0, im: 0.0 });
                self.delay_buffer
                    .resize(DECORR_DELAY_BUF_LEN_MPS_LD_1, Complex { re: 0.0, im: 0.0 });
            }
        }

        if reset_buffs {
            self.state_buffer.fill(Complex { re: 0.0, im: 0.0 });
            self.delay_buffer.fill(Complex { re: 0.0, im: 0.0 });
            self.rev_band_delay_buffer_index.fill(0);
        }

        self.num_bins = num_hyb_bands;
        let rev_filter_order = self.rev_filter_order.unwrap();
        let rev_band_offset = self.rev_band_offset.unwrap();
        let rev_delay = self.rev_delay.unwrap();

        let mut i_start = 0;

        // Memory for the delay_buffer is allocated in a consecutive order, so we
        // can address from filter to filter with a constant length.
        // The same is valid for the states.
        // All filters in a reverb band have the same filter coefficients.
        for ((rb, i_stop), num_sample_delay) in
            izip!(rev_band_offset.iter().enumerate(), rev_delay.iter(),).take(NUM_DECORR_BANDS)
        {
            if *i_stop <= i_start {
                continue;
            }

            match decorr_type {
                DecorrType::Ps => {
                    let mut band = i_start;
                    for flt in self.filter[i_start..*i_stop].iter_mut() {
                        flt.init(
                            DECORR_FILTER_ORDER_PS,
                            rb,
                            band,
                            *num_sample_delay,
                            decorr_type,
                            &mut offset_state_buff,
                            &mut offset_delay_buff,
                        );
                        band += 1;
                    }
                }
                _ => {
                    let reverb_filter_order = rev_filter_order[rb];
                    for flt in self.filter[i_start..*i_stop].iter_mut() {
                        flt.init(
                            reverb_filter_order,
                            rb,
                            0,
                            *num_sample_delay,
                            decorr_type,
                            &mut offset_state_buff,
                            &mut offset_delay_buff,
                        );
                    }
                }
            }

            i_start = *i_stop;
        }

        if (offset_state_buff > self.state_buffer.len())
            || (offset_delay_buff > self.delay_buffer.len())
        {
            error = 1;
            return error;
        }

        let ducker_type: DuckerType;
        let num_param_bands;
        match decorr_type {
            DecorrType::Ps => {
                ducker_type = DuckerType::Ps;
                num_param_bands = DecorrMaxParameterBands::Ps;
            }
            DecorrType::Usac => {
                ducker_type = DuckerType::Mps;
                num_param_bands = DecorrMaxParameterBands::Mps;
            }
            DecorrType::Ld => {
                ducker_type = DuckerType::Mps;
                num_param_bands = DecorrMaxParameterBands::Ld;
            }
        }
        error = self
            .ducker
            .init(self.num_bins, ducker_type, num_param_bands, reset_buffs);

        error
    }

    /// Creates a new `DecorrDec` instance.
    /// # Return
    /// `DecorrDec` instance.
    pub fn new() -> DecorrDec {
        DecorrDec {
            state_buffer: Default::default(),
            delay_buffer: Default::default(),
            rev_filter_type: None,
            rev_band_offset: None,
            rev_delay: None,
            rev_filter_order: None,
            rev_band_delay_buffer_index: Default::default(),
            state_buffer_offset: Default::default(),
            filter: [DecorrFilterInstance::new(); NUM_HYBRID_DATA_BANDS],
            ducker: DuckerInstance::new(),
            num_bins: 0,
        }
    }
}

impl Default for DecorrDec {
    fn default() -> Self {
        Self::new()
    }
}
