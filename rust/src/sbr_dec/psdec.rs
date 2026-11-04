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
//! Parametric stereo (PS) decoding process

use super::{
    common::SbrError,
    constants::MAX_EL_CHANNELS,
    psbitdec::{
        PsDecBsData, MAX_NO_PS_ENV, NO_ICC_STEPS, NO_IID_STEPS, NO_IID_STEPS_FINE, NO_MID_RES_BINS,
    },
};
use crate::common::{
    decorrelator::{DecorrDec, DecorrType},
    hybrid::{HybridAnalysis, HybridSynthesis, NUM_HYBRID_DATA_BANDS},
    qmf_domain::{QmfDomainQmfSlots, QmfDomainQmfSlotsMut},
};
use itertools::izip;
use num_complex::Complex;

/// Number of QMF sub-groups.
const SUBQMF_GROUPS: usize = 10;
/// Number of QMF groups.
const QMF_GROUPS: usize = 12;
/// Number of inter-channel intensity difference (IID) groups.
const NO_IID_GROUPS: usize = SUBQMF_GROUPS + QMF_GROUPS;
/// Number of QMF subbands for hybrid filter.
const NO_QMF_BANDS_HYBRID20: usize = 3;
/// Number of QMF subband channels.
const NO_QMF_CHANNELS: usize = 64;
/// Number of QMF sub subband channels.
const NO_SUB_QMF_CHANNELS: usize = 12;
/// Number of hybrid filter delay timeslots.
const HYBRID_FILTER_DELAY: usize = 6;
/// Number of inter-channel coherence (ICC) levels.
const NO_ICC_LEVELS: usize = NO_ICC_STEPS as usize;
/// Number of inter-channel intensity difference (IID) levels.
const NO_IID_LEVELS: usize = 2 * NO_IID_STEPS as usize + 1;
/// Number of inter-channel intensity difference (IID) fine levels.
const NO_IID_LEVELS_FINE: usize = 2 * NO_IID_STEPS_FINE as usize + 1;

/// Structure that holds parametric stereo coefficients.
#[repr(C)]
#[derive(Debug, Default)]
struct PsDecCoefficients {
    /// Coefficients of the sub-subband groups.
    h11r: [f32; NO_IID_GROUPS],
    h12r: [f32; NO_IID_GROUPS],
    h21r: [f32; NO_IID_GROUPS],
    h22r: [f32; NO_IID_GROUPS],

    delta_h11r: [f32; NO_IID_GROUPS],
    delta_h12r: [f32; NO_IID_GROUPS],
    delta_h21r: [f32; NO_IID_GROUPS],
    delta_h22r: [f32; NO_IID_GROUPS],

    /// The mapped IID index for all envelopes and all IID bins.
    iid_index_mapped: [[i8; NO_MID_RES_BINS]; MAX_NO_PS_ENV],
    /// The mapped ICC index for all envelopes and all ICC bins.
    icc_index_mapped: [[i8; NO_MID_RES_BINS]; MAX_NO_PS_ENV],
}

impl PsDecCoefficients {
    /// Creates a new `PsDecCoefficients` instance.
    fn new() -> Self {
        Default::default()
    }

    /// Applies slot based rotation.
    ///
    /// # Parameters
    ///
    /// - `hybrid_left`: Hybrid values for left channel.
    /// - `hybrid_right`: Hybrid values for right channel.
    fn apply_slot_based_rotation(
        &mut self,
        hybrid_left: &mut [Complex<f32>; NUM_HYBRID_DATA_BANDS],
        hybrid_right: &mut [Complex<f32>; NUM_HYBRID_DATA_BANDS],
    ) {
        // Mapping  (See ISO/IEC 14496-3:2009(E) - Subpart 8.6.4.6.1).
        //
        // The number of stereo bands that is actually used depends on the number of
        // available parameters for IID and ICC:
        // ----------------------------------------------------------
        // nr. of IID para.| nr. of ICC para. | nr. of Stereo bands |
        // ----------------|------------------|---------------------|
        // 10,20           |     10,20        |        20           |
        // 10,20           |     34           |        34           |
        // 34              |     10,20        |        34           |
        // 34              |     34           |        34           |
        // ----------------------------------------------------------
        // In the case the number of parameters for IIS and ICC differs from the number
        // of stereo bands, a mapping from the lower number to the higher number of
        // parameters is applied. Index mapping of IID and ICC parameters is already done
        // in `PsDecBsData`. Further mapping is not needed here in baseline version.
        //

        // Mixing (See ISO/IEC 14496-3:2009(E) - Subpart 8.6.4.6.2)
        //
        // To generate the QMF subband signals for the subband samples n = n[e]+1 ,,,
        // n_[e+1] the parameters at position n[e] and n[e+1] are required as well as the
        // subband domain signals s_k(n) and d_k(n) for n = n[e]+1... n_[e+1]. n[e]
        // represents the start position for envelope e. The border positions n[e] are
        // handled in DecodePS().
        //
        // The stereo sub subband signals are constructed as:
        //
        // l_k(n) = H11(k,n) s_k(n) + H21(k,n) d_k(n)
        // r_k(n) = H21(k,n) s_k(n) + H22(k,n) d_k(n)
        //
        // In order to obtain the matrices H11(k,n)... H22 (k,n), the vectors h11(b)...
        // h22(b) need to be calculated first (b: parameter index). Depending on ICC mode
        // either mixing procedure R_a or R_b is used for that. For both procedures, the
        // parameters for parameter position n[e+1] is used.
        //

        // Phase parameters (See ISO/IEC 14496-3:2009(E) - Subpart 8.6.4.6.3)
        //
        // With disabled phase parameters (which is the case in baseline version), the
        // H-matrices are just calculated by:
        //
        // H11(k,n[e+1] = h11(b(k))
        // (...)
        // b(k): parameter index according to mapping table
        //

        // Processing of the samples in the sub subbands.
        // This loop includes the interpolation of the coefficients Hxx.
        let mut start = GROUP_TABLE[0];
        for (
            stop,
            c_h11r,
            c_h12r,
            c_h21r,
            c_h22r,
            delta_h11r,
            delta_h12r,
            delta_h21r,
            delta_h22r,
        ) in izip!(
            GROUP_TABLE[1..].iter(),
            self.h11r.iter_mut(),
            self.h12r.iter_mut(),
            self.h21r.iter_mut(),
            self.h22r.iter_mut(),
            self.delta_h11r.iter(),
            self.delta_h12r.iter(),
            self.delta_h21r.iter(),
            self.delta_h22r.iter(),
        )
        .take(NO_IID_GROUPS)
        {
            *c_h11r += *delta_h11r;
            *c_h12r += *delta_h12r;
            *c_h21r += *delta_h21r;
            *c_h22r += *delta_h22r;

            for (hyb_l, hyb_r) in izip!(
                hybrid_left[usize::from(start)..].iter_mut(),
                hybrid_right[usize::from(start)..].iter_mut()
            )
            .take(usize::from(*stop - start))
            {
                // construct stereo sub subband signals according to:
                //
                // l_k(n) = H11(k,n) s_k(n) + H21(k,n) d_k(n)
                // r_k(n) = H12(k,n) s_k(n) + H22(k,n) d_k(n)
                //
                let tmp_left = ((*hyb_l) * *c_h11r) + ((*hyb_r) * *c_h21r);
                let tmp_right = ((*hyb_l) * *c_h12r) + ((*hyb_r) * *c_h22r);

                *hyb_l = tmp_left;
                *hyb_r = tmp_right;
            }

            start = *stop;
        }
    }
}

/// Structure that holds parametric stereo data.
#[repr(C)]
#[derive(Debug, Default)]
pub struct PsDec {
    /// Previous calculated h(xy) coefficients.
    h11r_prev: [f32; NO_IID_GROUPS],
    h12r_prev: [f32; NO_IID_GROUPS],
    h21r_prev: [f32; NO_IID_GROUPS],
    h22r_prev: [f32; NO_IID_GROUPS],
    /// Decorrelator.
    decorr: DecorrDec,
    /// PS data structure.
    bs_data: PsDecBsData,
    /// Hybrid analysis filterbank.
    hybrid_analysis: HybridAnalysis,
    /// Hybrid synthesis filterbank.
    hybrid_synthesis: [HybridSynthesis; 2],
}

impl PsDec {
    /// Creates a new `PsDec` instance.
    pub fn new() -> Self {
        Default::default()
    }

    /// Initializes the `PsDec` instance.
    ///
    /// # Parameters
    ///
    /// - `aac_samples_per_frame`: Number of samples per frame.
    ///
    /// # Return
    /// - `SbrError`
    pub fn init(&mut self, aac_samples_per_frame: u16) -> SbrError {
        // Create Analysis Hybrid filterbank.
        self.hybrid_analysis = HybridAnalysis::new(false);

        // initialisation.
        if self.bs_data.init_ps_data(aac_samples_per_frame) != SbrError::Ok {
            self.deinit();
            return SbrError::CreateError;
        }

        let error = self.reset();
        if error != SbrError::Ok {
            self.deinit();
            return SbrError::CreateError;
        }
        SbrError::Ok
    }

    /// De-allocates internal dynamic memories.
    pub fn deinit(&mut self) {
        self.decorr.deinit();
        self.hybrid_analysis.deallocate_hf_memory();
    }

    /// Returns immutable reference to bitstream data of `PsDec`.
    pub fn bs_data(&self) -> &PsDecBsData {
        &self.bs_data
    }

    /// Returns mutable reference to bitstream data of `PsDec`.
    pub fn bs_data_mut(&mut self) -> &mut PsDecBsData {
        &mut self.bs_data
    }

    /// Resets some values (hybrid filter banks, decorrelator and previous coefficients) of the
    /// `PsDec` to default states.
    ///
    /// # Return
    /// - `SbrError`
    fn reset(&mut self) -> SbrError {
        // Initialize Analysis Hybrid filterbank.
        self.hybrid_analysis.init(NO_QMF_BANDS_HYBRID20 as u8, true);

        // Initialize Synthesis Hybrid filterbank.
        for hyb_syn in self.hybrid_synthesis.iter_mut() {
            hyb_syn.init(NO_QMF_CHANNELS);
        }

        // Initialize decorrelator.
        let error = self.decorr.init(71, DecorrType::Ps, 0, true);
        if error != 0 {
            SbrError::NotInitialized
        } else {
            self.h11r_prev.fill(1.0_f32);
            self.h12r_prev.fill(1.0_f32);

            self.h21r_prev.fill(0.0_f32);
            self.h22r_prev.fill(0.0_f32);

            SbrError::Ok
        }
    }

    /// Feeds delaylines when parametric stereo is switched on.
    ///
    /// # Parameters
    ///
    /// - `qmf_domain_slots_left`: `QmfDomainQmfSlots` instance with valid internal data.
    pub(super) fn feed_delay_lines(&mut self, qmf_domain_slots_left: &QmfDomainQmfSlots) {
        // Fill hybrid delay buffer.
        let mut hybrid = [Complex {
            re: 0.0_f32,
            im: 0.0_f32,
        }; NO_SUB_QMF_CHANNELS];

        for (qmf_real, qmf_imag) in izip!(
            qmf_domain_slots_left.real.iter(),
            qmf_domain_slots_left.imag.iter()
        )
        .take(HYBRID_FILTER_DELAY)
        {
            self.hybrid_analysis.apply(qmf_real, qmf_imag, &mut hybrid);
        }
    }

    /// Initializes slot based rotation and updates parametric stereo coefficients.
    ///
    /// # Parameters
    ///
    /// - `psdec_coeffs`: Parametric stereo coefficients data.
    /// - `env`: Index value for envelope to be used.
    fn init_slot_based_rotation(&mut self, psdec_coeffs: &mut PsDecCoefficients, env: usize) {
        const FL_SQRT05: f32 = 0.70710678118_f32;

        let (scale_factors, num_iid_steps) = if self.bs_data.fine_iid_quant() {
            (&SCALE_FACTORS_FINE[..], NO_IID_STEPS_FINE as i8)
        } else {
            (&SCALE_FACTORS[..], NO_IID_STEPS as i8)
        };

        // Dequantize and decode.
        let iid_index_mapped = &psdec_coeffs.iid_index_mapped[env];
        let icc_index_mapped = &psdec_coeffs.icc_index_mapped[env];

        // inv_l = 1/(length of envelope)
        let l1 = self.bs_data.env_start_stop(env);
        let l2 = self.bs_data.env_start_stop(env + 1);
        let inv_l = 1.0_f32 / (l2 - l1) as f32;

        // Type 'A' rotation:
        // mixing procedure Ra, used in baseline version.
        //
        // Scale-factor vectors c1 and c2 are precalculated and stored in SCALE_FACTORS[] and
        // SCALE_FACTORS_FINE[]. From the linearized IID parameters (intensity differences),
        // two scale factors are calculated. They are used to obtain the coefficients h11... h22
        // (See ISO/IEC 14496-3:2009(E) - Subpart 8.6.4.6.2.1).

        // Dequantize and decode.
        for (
            bin,
            c_h11r,
            c_h12r,
            c_h21r,
            c_h22r,
            delta_h11r,
            delta_h12r,
            delta_h21r,
            delta_h22r,
            h11r_prev,
            h12r_prev,
            h21r_prev,
            h22r_prev,
        ) in izip!(
            BINS2GROUP_MAP20.iter(),
            psdec_coeffs.h11r.iter_mut(),
            psdec_coeffs.h12r.iter_mut(),
            psdec_coeffs.h21r.iter_mut(),
            psdec_coeffs.h22r.iter_mut(),
            psdec_coeffs.delta_h11r.iter_mut(),
            psdec_coeffs.delta_h12r.iter_mut(),
            psdec_coeffs.delta_h21r.iter_mut(),
            psdec_coeffs.delta_h22r.iter_mut(),
            self.h11r_prev.iter_mut(),
            self.h12r_prev.iter_mut(),
            self.h21r_prev.iter_mut(),
            self.h22r_prev.iter_mut(),
        )
        .take(NO_IID_GROUPS)
        {
            let iid_index = iid_index_mapped[usize::from(*bin)];
            let scf_index_r = usize::try_from(num_iid_steps + iid_index).unwrap();
            let scf_index_l = usize::try_from(num_iid_steps - iid_index).unwrap();
            let scale_r = scale_factors[scf_index_r];
            let scale_l = scale_factors[scf_index_l];

            let icc_index = usize::try_from(icc_index_mapped[usize::from(*bin)]).unwrap();
            let alpha = ALPHAS[icc_index];
            let beta = (alpha * (scale_r - scale_l)) * FL_SQRT05;

            // Calculate the coefficients h11... h22 from scale-factors and ICC parameters.
            let x1 = beta + alpha;
            let x2 = beta - alpha;
            let h11r = scale_l * x1.cos();
            let h12r = scale_r * x2.cos();
            let h21r = scale_l * x1.sin();
            let h22r = scale_r * x2.sin();

            // Interpolation:
            // (See ISO/IEC 14496-3:2009(E) - Subpart 8.6.4.6.4).
            // Interpolation of the matrices H11... H22:
            //
            // H11(k,n) = H11(k,n[e]) + ((n-n[e]) * ((H11(k,n[e+1]) - H11(k,n[e]) / (n[e+1] -
            // n[e])))
            // ... same for H12, H21, H22.

            *c_h11r = *h11r_prev;
            *c_h12r = *h12r_prev;
            *c_h21r = *h21r_prev;
            *c_h22r = *h22r_prev;

            *delta_h11r = (h11r - *c_h11r) * inv_l;
            *delta_h12r = (h12r - *c_h12r) * inv_l;
            *delta_h21r = (h21r - *c_h21r) * inv_l;
            *delta_h22r = (h22r - *c_h22r) * inv_l;

            // Update prev coefficients for interpolation in next envelope.
            *h11r_prev = h11r;
            *h12r_prev = h12r;
            *h21r_prev = h21r;
            *h22r_prev = h22r;
        }
    }

    /// Applies IID, ICC, IPD and OPD parameters to the current frame.
    ///
    /// # Parameters
    ///
    /// - `psdec_coeffs`: Parametric stereo coefficients.
    /// - `lbsc_real`: Real band slot of left qmf channel.
    /// - `lbsc_imag`: Imaginary band slot of left qmf channel.
    /// - `lbsc_delayed_real`: Real band slot of left qmf channel delayed by `HYBRID_FILTER_DELAY`
    ///   number of slots.
    /// - `lbsc_delayed_imag`: Imaginary band slot of left qmf channel delayed by
    /// - `lbsrc_real`: Real band slot of right qmf channel.
    /// - `lbsrc_imag`: Imaginary band slot of right qmf channel.
    #[expect(clippy::too_many_arguments)]
    fn apply_ps_slot(
        &mut self,
        psdec_coeffs: &mut PsDecCoefficients,
        lbsc_real: &mut [f32],
        lbsc_imag: &mut [f32],
        lbsc_delayed_real: &[f32],
        lbsc_delayed_imag: &[f32],
        lbsrc_real: &mut [f32],
        lbsrc_imag: &mut [f32],
    ) {
        // The 64-band QMF representation of the monaural signal generated by the SBR tool
        // is used as input of the PS tool. After the PS processing, the outputs of the
        // left and right hybrid synthesis filterbanks are used to generate the stereo
        // output signal.
        //
        //            -------------            ----------            -------------
        //           | Hybrid      | M_n[k,m] |          | L_n[k,m] | Hybrid      | l[n]
        //  m[n] --->| analysis    |--------->|          |--------->| synthesis   |----->
        //            -------------           | Stereo   |           -------------
        //                  |                 | recon-   |
        //                  |                 | stuction |
        //                 \|/                |          |
        //            -------------           |          |
        //           | De-         | D_n[k,m] |          |
        //           | correlation |--------->|          |
        //            -------------           |          |           -------------
        //                                    |          | R_n[k,m] | Hybrid      | r[n]
        //                                    |          |--------->| synthesis   |----->
        //  IID, ICC ------------------------>|          |          | filter bank |
        // (IPD, OPD)                          ----------            -------------
        //
        // m[n]: QMF representation of the mono input
        // M_n[k,m]: (sub-)sub-band domain signals of the mono input
        // D_n[k,m]: decorrelated (sub-)sub-band domain signals
        // L_n[k,m]: (sub-)sub-band domain signals of the left output
        // R_n[k,m]: (sub-)sub-band domain signals of the right output
        // l[n],r[n]: left/right output signals

        // Hybrid data for left channel.
        let mut hyb_data_left = [Complex {
            re: 0.0_f32,
            im: 0.0_f32,
        }; NUM_HYBRID_DATA_BANDS];

        // Hybrid data for right channel.
        let mut hyb_data_right = hyb_data_left;

        // Hybrid analysis filterbank:
        // The lower 3 (5) of the 64 QMF subbands are further split to provide better
        // frequency resolution. for PS processing. For the 10 and 20 stereo bands
        // configuration, the QMF band H_0(w) is split up into 8 (sub-) sub-bands and the
        // QMF bands H_1(w) and H_2(w) are spit into 2 (sub-) 4th. (See figures 8.20
        // and 8.22 of ISO/IEC 14496-3:2001/FDAM 2:2004(E))

        // LF - part.
        self.hybrid_analysis
            .apply(lbsc_delayed_real, lbsc_delayed_imag, &mut hyb_data_left);

        // HF - part. Bands up to lsb.
        for (hyb_data, lbc_real, lbc_imag) in izip!(
            hyb_data_left[NO_SUB_QMF_CHANNELS - 2..].iter_mut(),
            lbsc_real[NO_QMF_BANDS_HYBRID20..].iter(),
            lbsc_imag[NO_QMF_BANDS_HYBRID20..].iter()
        )
        .take(NO_QMF_CHANNELS - NO_QMF_BANDS_HYBRID20)
        {
            hyb_data.re = *lbc_real;
            hyb_data.im = *lbc_imag;
        }

        // Decorrelation:
        // By means of all-pass filtering and delaying, the (sub-)sub-band samples s_k(n)
        // are converted into de-correlated (sub-)sub-band samples d_k(n).
        // - k: frequency in hybrid spectrum
        // - n: time index
        self.decorr.apply(&hyb_data_left, &mut hyb_data_right, 0);

        // Stereo Processing:
        // The sets of (sub-)sub-band samples s_k(n) and d_k(n) are processed according
        // to the stereo cues which are defined per stereo band.
        psdec_coeffs.apply_slot_based_rotation(&mut hyb_data_left, &mut hyb_data_right);

        // Hybrid synthesis filterbank:
        // The stereo processed hybrid subband signals l_k(n) and r_k(n) are fed into the
        // hybrid synthesis filterbanks which are identical to the 64 complex synthesis
        // filterbank of the SBR tool. The input to the filterbank are slots of 64 QMF
        // samples. For each slot the filterbank outputs one block of 64 samples of one
        // reconstructed stereo channel. The hybrid synthesis filterbank is computed
        // seperatly for the left and right channel.

        // Hybrid synthesis for left channel.
        self.hybrid_synthesis[0].apply(&hyb_data_left[..], lbsc_real, lbsc_imag);

        // Hybrid synthesis for right channel.
        self.hybrid_synthesis[1].apply(&hyb_data_right[..], lbsrc_real, lbsrc_imag);
    }

    /// Applies parametric stereo processing for a given `low_bands`.
    ///
    /// # Parameters
    ///
    /// - `low_bands`: Mutable reference to `[QmfDomainQmfSlotsMut; SBRDEC_MAX_EL_CHANNELS]`.
    /// - `num_time_slots`: Number of synthesis `QMF` time slots.
    pub(super) fn apply(
        &mut self,
        low_bands: &mut [QmfDomainQmfSlotsMut; MAX_EL_CHANNELS],
        num_time_slots: u8,
    ) {
        let (low_bands_left, low_bands_right) = low_bands.split_at_mut(1);

        let mut psdec_coeffs = PsDecCoefficients::new();

        self.bs_data.fill_ps_data(
            &mut psdec_coeffs.iid_index_mapped,
            &mut psdec_coeffs.icc_index_mapped,
        );

        let lbc_ch0 = &mut low_bands_left[0];
        let lbc_ch1 = &mut low_bands_right[0];

        let mut env = 0;
        for slot in 0..usize::from(num_time_slots) {
            let (lbc_real, lbc_delayed_real) =
                lbc_ch0.real.split_at_mut(HYBRID_FILTER_DELAY + slot);
            let (lbc_imag, lbc_delayed_imag) =
                lbc_ch0.imag.split_at_mut(HYBRID_FILTER_DELAY + slot);

            if slot as u8 == self.bs_data.env_start_stop(env) {
                self.init_slot_based_rotation(&mut psdec_coeffs, env);
                env += 1;
            }

            self.apply_ps_slot(
                &mut psdec_coeffs,
                lbc_real[slot],
                lbc_imag[slot],
                lbc_delayed_real[0],
                lbc_delayed_imag[0],
                lbc_ch1.real[slot],
                lbc_ch1.imag[slot],
            );
        }
    }
}

#[rustfmt::skip]
static BINS2GROUP_MAP20: [u8; NO_IID_GROUPS] = [
 0, 0, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
];

#[rustfmt::skip]
static GROUP_TABLE:[u8; NO_IID_GROUPS + 1] = [
 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
 12, 13, 14, 15, 16, 18, 21, 25, 30, 42, 71 ];

#[rustfmt::skip]
static ALPHAS: [f32; NO_ICC_LEVELS] = [
 0.000000000, 0.178427666, 0.285667300, 0.463072360,
 0.597163141, 0.785398185, 1.100308537, 1.570796371 ];

#[rustfmt::skip]
static SCALE_FACTORS: [f32; NO_IID_LEVELS] = [
 1.411982775, 1.403138161, 1.386876702, 1.348399758,
 1.291249394, 1.196037412, 1.107372403, 1.000000000,
 0.879617155, 0.754648566, 0.576779902, 0.426401436,
 0.276718289, 0.176644623, 0.079401627 ];

#[rustfmt::skip]
static SCALE_FACTORS_FINE: [f32; NO_IID_LEVELS_FINE] = [
 1.414206505, 1.414191246, 1.414142847, 1.413990021,
 1.413506985, 1.411982775, 1.409772992, 1.405394793,
 1.396779656, 1.380053043, 1.348399758, 1.313920140,
 1.264310122, 1.196037412, 1.107372403, 1.000000000,
 0.879617155, 0.754648566, 0.633656085, 0.523081064,
 0.426401436, 0.308955401, 0.221374646, 0.157687888,
 0.111982249, 0.079401627, 0.044699017, 0.025144693,
 0.014141428, 0.007952581, 0.004472114 ];
