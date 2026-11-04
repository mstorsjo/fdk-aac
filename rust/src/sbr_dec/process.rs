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
//! Spectral band replication (SBR) decoding process.

// Imports
use super::{
    common::{self as sbr_common, SbrError},
    constants::{
        LPC_ORDER, MAX_COLS, MAX_EL_CHANNELS, MAX_NUM_PATCHES, MAX_OV_COLS, PVC_NTIMESLOT,
        SBRDEC_MAX_SUBBAND_SUBSAMPLES,
    },
    drc::SbrDecDrcChannel,
    env_calc::EnvelopeData,
    flags::SbrDecFlags,
    frame_data::{
        constants::{HI, LO},
        FrameData, PrevFrameData, SbrPatchingMode,
    },
    hbe::{HbeTransposer, KeepStatesSyncedMode, HBE_MAX_NUM_PATCHES, HBE_MAX_STRETCH},
    header_data::HeaderData,
    lpp_trans::{LppTransposer, TransposerSettings},
    psdec::PsDec,
    pvc::{Mode, Ns, PvcDecoder, Rate},
};
use crate::common::{
    qmf::QMF_MAX_SYNTHESIS_SUBBANDS,
    qmf_domain::{
        QmfDomain, QmfDomainOvQmfSlotsMut, QmfDomainQmfSlots, QmfDomainQmfSlotsMut, QmfDomainSelect,
    },
};
use itertools::izip;
use num_complex::Complex;
use std::array;

#[repr(C)]
#[derive(Default, Debug, PartialEq, Copy, Clone)]
pub enum SbrDecEsbrMode {
    /// eSbrMode: HBE processing is off.
    #[default]
    Off,
    /// eSbrMode: HBE processing is on.
    On,
    /// eSbrMode: start HBE processing.
    Start,
    /// eSbrMode: no states update required for next frame.
    Pause,
}

/// Structure which holds internal sub-components for SBR decoding process.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct SbrDec {
    /// SBR envelope calculate instance.
    calc_envelope: EnvelopeData,
    /// LPP transposer instance.
    lpp_trans: LppTransposer,
    /// PVC decoder instance.
    pvc_data: Option<Box<PvcDecoder>>,
    /// Codec frame size.
    codec_frame_size: u16,
    /// HBE transposer instance.
    hbe: Option<Box<HbeTransposer>>,
    /// Channel number in qmf domain.
    ch_qmf_domain: u8,
    /// SBR DRC channel instance.
    sbr_drc_channel: SbrDecDrcChannel,
    /// Flag to indicate filter states are saved or not.
    are_states_saved: bool,
    /// ESBR mode.
    esbr_mode: SbrDecEsbrMode,
    /// SBR patching mode.
    patching_mode: SbrPatchingMode,
    /// Flag to store sbr frame process status.
    apply_sbr_proc_old: bool,
}

impl SbrDec {
    /// Creates a new instance of `SbrDec`.
    ///
    /// # Return
    ///
    /// A new instance of `SbrDec` with default values.
    pub fn new() -> Self {
        Default::default()
    }

    /// Returns mutable reference to `self.calc_envelope`.
    pub fn calc_envelope_mut(&mut self) -> &mut EnvelopeData {
        &mut self.calc_envelope
    }

    /// Returns Option<&mut PvcDecoder>.
    pub fn pvc_data_mut(&mut self) -> Option<&mut PvcDecoder> {
        if let Some(pvc_data) = &mut self.pvc_data {
            Some(pvc_data)
        } else {
            None
        }
    }

    /// Returns Option<&PvcDecoder>.
    pub fn pvc_data(&self) -> Option<&PvcDecoder> {
        if let Some(pvc_data) = &self.pvc_data {
            Some(pvc_data)
        } else {
            None
        }
    }

    /// Returns Option<&mut HbeTransposer>.
    pub fn hbe_mut(&mut self) -> Option<&mut HbeTransposer> {
        if let Some(hbe) = &mut self.hbe {
            Some(hbe)
        } else {
            None
        }
    }

    /// Returns Option<&HbeTransposer>.
    pub fn hbe(&self) -> Option<&HbeTransposer> {
        if let Some(hbe) = &self.hbe {
            Some(hbe)
        } else {
            None
        }
    }

    /// Returns mutable reference to `self.sbr_drc_channel`.
    pub fn sbr_drc_channel_mut(&mut self) -> &mut SbrDecDrcChannel {
        &mut self.sbr_drc_channel
    }

    /// Returns immutable reference to `self.sbr_drc_channel`.
    pub fn sbr_drc_channel(&self) -> &SbrDecDrcChannel {
        &self.sbr_drc_channel
    }

    /// Returns channel number in qmf domain.
    pub fn ch_qmf_domain(&self) -> u8 {
        self.ch_qmf_domain
    }

    /// Sets channel for qmf domain.
    pub fn set_ch_qmf_domain(&mut self, channel: u8) {
        self.ch_qmf_domain = channel;
    }

    /// Returns `eSBR` mode value.
    pub fn esbr_mode(&self) -> SbrDecEsbrMode {
        self.esbr_mode
    }

    /// Sets `eSBR` mode value.
    pub fn set_esbr_mode(&mut self, esbr_mode: SbrDecEsbrMode) {
        self.esbr_mode = esbr_mode;
    }

    /// Applies QMF analysis filtering.
    ///
    /// # Parameters
    ///
    /// - `qd`: QMF domain (`QmfDomain`) instance.
    /// - `time_in`: Input time domain signal.
    /// - `patching_mode`: SBR patching mode.
    /// - `flags`: SBR decoder flags.
    pub fn analysis(
        &mut self,
        qd: &mut QmfDomain,
        time_in: Option<&[f32]>,
        patching_mode: SbrPatchingMode,
        flags: SbrDecFlags,
    ) {
        // Number of QMF slots per frame.
        let num_cols = usize::from(qd.num_qmf_time_slots());
        let channel = usize::from(self.ch_qmf_domain);

        if flags.contains(SbrDecFlags::USAC_HARMONICSBR) {
            if let Some(hbe) = &mut self.hbe {
                debug_assert!(num_cols == hbe.no_cols() as usize);

                let hbe_buffer = &mut hbe.hbe_buffer;
                if flags.contains(SbrDecFlags::USAC_HBE_LP) {
                    if !flags.contains(SbrDecFlags::SKIP_QMF_ANA) {
                        // Calculate qmf analysis filterbank.
                        //    - store qmf data in workbuffer.
                        //    - low_band->real/imag[num_ov_ts..num_ov_ts + num_cols].

                        qd.qmf_analysis_filtering(
                            hbe_buffer.hbe_light_time_delay_buffer(),
                            channel,
                        );
                    }

                    // Delay the aac signal by one frame to compensate the HBE transposer delay.
                    hbe_buffer.hbe_light_time_delay_buffer_mut()
                        [..usize::from(self.codec_frame_size)]
                        .copy_from_slice(
                            &time_in.as_ref().unwrap()[..usize::from(self.codec_frame_size)],
                        );
                } else {
                    let (real_buffer, imag_buffer, time_delay_buffer) = hbe_buffer.buffers_mut();

                    let mut real_slots: [&mut [f32]; MAX_COLS] =
                        array::from_fn(|_| [].as_mut_slice());
                    let mut imag_slots: [&mut [f32]; MAX_COLS] =
                        array::from_fn(|_| [].as_mut_slice());

                    sbr_common::get_slices_out_of_box_of_box_mut(
                        &mut real_buffer[num_cols..2 * num_cols],
                        &mut real_slots,
                    );
                    sbr_common::get_slices_out_of_box_of_box_mut(
                        &mut imag_buffer[num_cols..2 * num_cols],
                        &mut imag_slots,
                    );

                    if !flags.contains(SbrDecFlags::SYNTAX_USAC)
                        && self.esbr_mode == SbrDecEsbrMode::Start
                    {
                        // Drain HBE light time delay buffer if eSBR mode is
                        // `SbrDecEsbrMode::Start`.

                        qd.qmf_analysis_filtering_cplx(
                            &mut real_slots[..num_cols],
                            &mut imag_slots[..num_cols],
                            time_delay_buffer,
                            channel,
                        );

                        // Set eSBR mode to full harmonic qmf transposer.
                        self.esbr_mode = SbrDecEsbrMode::On;
                    }

                    if flags.contains(SbrDecFlags::SKIP_QMF_ANA) {
                        // Exchange qmf data of hbe buffer and workbuffer in case of
                        // stereo_cfg_index = 3 with HBE:
                        // hbe_buffer->real/imag[num_cols..2*num_cols] <=>
                        // low_band->real/imag[num_ov_ts..num_ov_ts + num_cols]
                        //  - Move old hbe data to workbuffer
                        //  - Move new hbe data to hbe buffer

                        qd.qmf_data_to_hbe(
                            &mut real_slots[..num_cols],
                            &mut imag_slots[..num_cols],
                            channel,
                        );
                    } else {
                        {
                            // Move old qmf data from hbe buffer to workbuffer
                            // hbe_buffer->real/imag[num_cols..2*num_cols] =>
                            // low_band->real/imag[num_ov_ts..num_ov_ts + num_cols]

                            let mut real_slots_immut: [&[f32]; MAX_COLS] =
                                array::from_fn(|_| [].as_slice());
                            let mut imag_slots_immut: [&[f32]; MAX_COLS] =
                                array::from_fn(|_| [].as_slice());

                            sbr_common::convert_mut_to_immut_slices(
                                &real_slots[..num_cols],
                                &mut real_slots_immut,
                            );
                            sbr_common::convert_mut_to_immut_slices(
                                &imag_slots[..num_cols],
                                &mut imag_slots_immut,
                            );

                            qd.hbe_to_qmf_data(
                                &real_slots_immut[..num_cols],
                                &imag_slots_immut[..num_cols],
                                channel,
                            );
                        }

                        // Calculate qmf analysis filterbank.
                        // - store qmf data in hbe buffer.
                        // - hbe_buffer>real/imag[num_cols..2*num_cols].

                        qd.qmf_analysis_filtering_cplx(
                            &mut real_slots[..num_cols],
                            &mut imag_slots[..num_cols],
                            time_in.as_ref().unwrap(),
                            channel,
                        );
                    }
                }
            }
        } else if !flags.contains(SbrDecFlags::SKIP_QMF_ANA) {
            // Calculate qmf analysis filterbank.
            // - store qmf data in workbuffer.
            // - low_band->real/imag[num_ov_ts..num_ov_ts + num_cols].
            qd.qmf_analysis_filtering(time_in.as_ref().unwrap(), channel);
        }

        // Clear upper half of spectrum.
        if !(flags.contains(SbrDecFlags::USAC_HARMONICSBR)
            && (patching_mode == SbrPatchingMode::Hbe))
            || flags.contains(SbrDecFlags::USAC_HBE_LP)
        {
            // Clear qmf data above analysis bands.
            // low_band->real/imag[num_ov_ts..num_ov_ts + num_cols][num_analysis_bands..
            // QMF_DOMAIN_MAX_SYNTHESIS_QMF_BANDS].

            qd.clear_hbe_qmf_data(channel);
        }
    }

    /// Applies DRC factors frame based.
    ///
    /// # Parameters
    ///
    /// - `qd`: QMF domain (`QmfDomain`) instance.
    /// - `flags`: SBR decoder flags.
    pub fn drc_apply(&mut self, qd: &mut QmfDomain, flags: SbrDecFlags) {
        if !flags.contains(SbrDecFlags::SKIP_QMF_SYN) {
            let num_qmf_slots = usize::from(qd.num_qmf_time_slots());
            let mut low_band_slots = QmfDomainQmfSlotsMut::default();

            qd.get_pointer_array_for_qmf_slots_mut(
                &mut low_band_slots,
                usize::from(self.ch_qmf_domain),
            );

            self.sbr_drc_channel.apply(
                &mut low_band_slots.real[..num_qmf_slots],
                &mut low_band_slots.imag[..num_qmf_slots],
                num_qmf_slots as u32,
            );

            self.sbr_drc_channel.update_channel(flags);
        }
    }

    /// Applies QMF synthesis filtering.
    ///
    /// # Parameters
    ///
    /// - `qd`: QMF domain (`QmfDomain`) instance.
    /// - `time_out`: Output time domain signal.
    /// - `ov_usb`: Mutable reference to overlap usb.
    /// - `flags`: SBR decoder flags.
    pub fn synthesis(
        &mut self,
        qd: &mut QmfDomain,
        mut time_out: Option<&mut [f32]>,
        ov_usb: &mut u8,
        flags: SbrDecFlags,
    ) {
        if !flags.contains(SbrDecFlags::SKIP_QMF_SYN) {
            let channel = usize::from(self.ch_qmf_domain);

            // Apply qmf synthesis.
            qd.qmf_synthesis_filtering(time_out.as_mut().unwrap(), ov_usb, channel);

            // Update overlap buffer.
            // Even bands above usb are copied to avoid outdated spectral data in case
            // the stop frequency raises.

            qd.save_overlap(channel);
        }

        self.are_states_saved = false;
    }

    /// Applies parametric stereo (PS) decoding.
    ///
    /// # Parameters
    ///
    /// - `left_sbr_dec`: SBR decoder (`SbrDec`) instance for left channel.
    /// - `qd`: QMF domain (`QmfDomain`) instance.
    /// - `ps_dec`: Parametric stereo decoder (`PsDec`) instance.
    /// - `is_ps_applied`: Flag indicates whether last frame did contain PS data.
    pub fn parametric_stereo(
        &mut self,
        left_sbr_dec: &SbrDec,
        qd: &mut QmfDomain,
        ps_dec: &mut PsDec,
        is_ps_applied: bool,
    ) {
        let right_channel = usize::from(self.ch_qmf_domain);
        let left_channel = usize::from(left_sbr_dec.ch_qmf_domain);
        if !is_ps_applied {
            // Switching to PS applying.

            // Copy filter states from left to right channel.
            debug_assert!(qd.num_bands_synthesis() <= QMF_MAX_SYNTHESIS_SUBBANDS as u16);

            qd.cp_filter_states(right_channel, left_channel);

            let mut low_bands_left = QmfDomainQmfSlots::default();
            qd.get_pointer_array_for_qmf_slots(&mut low_bands_left, left_channel);

            // Feed delay lines.
            ps_dec.feed_delay_lines(&low_bands_left);
        }

        // Use the same synthesis qmf values for left and right channel.
        if let Some(syn_qmf_left) =
            qd.get_filter_bank_mut(left_channel, QmfDomainSelect::QmfDomainOut)
        {
            let lsb = syn_qmf_left.num_lf_subbands();
            let usb = syn_qmf_left.num_hf_subbands();
            let num_cols = syn_qmf_left.num_time_slots();

            if let Some(syn_qmf_right) =
                qd.get_filter_bank_mut(right_channel, QmfDomainSelect::QmfDomainOut)
            {
                syn_qmf_right.set_num_lf_subbands(lsb);
                syn_qmf_right.set_num_hf_subbands(usb);
                syn_qmf_right.set_num_time_slots(num_cols);

                let mut low_bands: [QmfDomainQmfSlotsMut; MAX_EL_CHANNELS] = Default::default();
                qd.get_pointer_array_for_qmf_slots_two_channels_mut(
                    &mut low_bands,
                    left_channel,
                    right_channel,
                );

                // Copy DRC data to right channel (with PS both channels use the same DRC gains).
                self.sbr_drc_channel = left_sbr_dec.sbr_drc_channel;

                ps_dec.apply(&mut low_bands, num_cols);
            }
        }
    }

    /// Applies transposing of QMF spectrum.
    ///
    /// # Parameters
    ///
    /// - `qd`: QMF domain (`QmfDomain`) instance.
    /// - `header_data`: SBR header data (`HeaderData`) instance.
    /// - `frame_data`: SBR frame data (`FrameData`) instance.
    /// - `prev_frame_data`: SBR previous frame data (`PrevFrameData`) instance.
    /// - `lpp_trans_settings`: LPP transposer settings (`TransposerSettings`) instance.
    /// - `flags`: SBR decoder flags.
    /// - `apply_processing`: Indicates whether sbr processing is applied.
    #[expect(clippy::too_many_arguments)]
    pub fn transposition(
        &mut self,
        qd: &mut QmfDomain,
        header_data: &mut HeaderData,
        frame_data: &FrameData,
        prev_frame_data: &PrevFrameData,
        lpp_trans_settings: &mut TransposerSettings,
        flags: SbrDecFlags,
        apply_processing: bool,
    ) {
        let is_usac_harmonic_sbr = flags.contains(SbrDecFlags::USAC_HARMONICSBR);
        let is_usac_hbe_lp = flags.contains(SbrDecFlags::USAC_HBE_LP);

        if apply_processing {
            let num_ov_cols = usize::from(lpp_trans_settings.overlap_size());
            let num_cols =
                usize::from(header_data.number_time_slots()) * usize::from(header_data.time_step());
            let last_slot_offs = usize::from(
                frame_data.frame_info.borders()[frame_data.frame_info.num_envelopes()]
                    - header_data.number_time_slots(),
            );

            if flags.contains(SbrDecFlags::SYNTAX_USAC) {
                if let Some(pvc) = &mut self.pvc_data {
                    let pvc_mode = if frame_data.frame_error_flag() {
                        // Legacy SBR mode.
                        Mode::None
                    } else {
                        header_data.bitstream_info().pvc_mode()
                    };

                    let ns = frame_data.ns();
                    let ns_option = if ns == 0 { None } else { Some(Ns::from(ns)) };

                    let urate = usize::from(header_data.time_step());
                    let rate = Rate::from(urate);

                    let mut low_band = QmfDomainQmfSlots::default();
                    qd.get_pointer_array_for_qmf_slots(
                        &mut low_band,
                        usize::from(self.ch_qmf_domain),
                    );

                    pvc.decode(
                        &low_band.real[..urate * PVC_NTIMESLOT as usize],
                        &low_band.imag[..urate * PVC_NTIMESLOT as usize],
                        frame_data.pvc_id(),
                        pvc_mode,
                        ns_option,
                        rate,
                        header_data.freq_band_data().start_subband,
                        frame_data.frame_info.pvc_borders()[0],
                    );
                }
            }

            // Inverse filtering of lowband and transposition into the SBR-frequency range.
            if !is_usac_hbe_lp {
                let mut kssm = if is_usac_harmonic_sbr
                    && frame_data.sbr_patching_mode() != SbrPatchingMode::Hbe
                {
                    KeepStatesSyncedMode::Normal
                } else {
                    KeepStatesSyncedMode::Off
                };

                if is_usac_harmonic_sbr {
                    if let Some(hbe) = &mut self.hbe {
                        // Code encapsulation to limit lifetime of hbe_buffer. Lifetime of
                        // hbe_buffer must end before calling hbe.apply().
                        {
                            let num_channels = usize::from(hbe.no_channels());
                            let hbe_buffer = &mut hbe.hbe_buffer;
                            let (real_buffer, imag_buffer, _time_delay_buffer) =
                                hbe_buffer.buffers_mut();

                            let mut real_slots: [&mut [f32]; MAX_COLS] =
                                array::from_fn(|_| [].as_mut_slice());
                            let mut imag_slots: [&mut [f32]; MAX_COLS] =
                                array::from_fn(|_| [].as_mut_slice());

                            sbr_common::get_slices_out_of_box_of_box_mut(
                                &mut real_buffer[..num_cols],
                                &mut real_slots,
                            );
                            sbr_common::get_slices_out_of_box_of_box_mut(
                                &mut imag_buffer[..num_cols],
                                &mut imag_slots,
                            );

                            if !self.are_states_saved
                                && frame_data.sbr_patching_mode() == SbrPatchingMode::Lpp
                            {
                                // Save states from previous frame in legacy lpc filterstate buffer.
                                let mut real_slots_immut: [&[f32]; MAX_OV_COLS + LPC_ORDER] =
                                    array::from_fn(|_| [].as_slice());
                                let mut imag_slots_immut: [&[f32]; MAX_OV_COLS + LPC_ORDER] =
                                    array::from_fn(|_| [].as_slice());

                                sbr_common::convert_mut_to_immut_slices(
                                    &real_slots[num_cols - LPC_ORDER - num_ov_cols..num_cols],
                                    &mut real_slots_immut,
                                );
                                sbr_common::convert_mut_to_immut_slices(
                                    &imag_slots[num_cols - LPC_ORDER - num_ov_cols..num_cols],
                                    &mut imag_slots_immut,
                                );

                                self.lpp_trans.save_leg_lpc_filter_states(
                                    &real_slots_immut[..LPC_ORDER + num_ov_cols],
                                    &imag_slots_immut[..LPC_ORDER + num_ov_cols],
                                    LPC_ORDER + num_ov_cols,
                                    num_channels,
                                );
                            }

                            // Saving unmodified QMF states in case of switching from legacy SBR to
                            // HBE.
                            let mut low_band = QmfDomainQmfSlots::default();
                            qd.get_pointer_array_for_qmf_slots(
                                &mut low_band,
                                usize::from(self.ch_qmf_domain),
                            );

                            {
                                for (hbe_re, hbe_im, lb_re, lb_im) in izip!(
                                    real_slots.iter_mut(),
                                    imag_slots.iter_mut(),
                                    low_band.real[num_ov_cols..].iter(),
                                    low_band.imag[num_ov_cols..].iter()
                                )
                                .take(num_cols)
                                {
                                    hbe_re[..num_channels].copy_from_slice(&lb_re[..num_channels]);
                                    hbe_im[..num_channels].copy_from_slice(&lb_im[..num_channels]);
                                }
                            }
                            if self.esbr_mode == SbrDecEsbrMode::Pause
                                && kssm == KeepStatesSyncedMode::Normal
                            {
                                kssm = KeepStatesSyncedMode::NormalSkip;
                            }
                        }

                        let mut low_band = QmfDomainQmfSlotsMut::default();
                        qd.get_pointer_array_for_qmf_slots_mut(
                            &mut low_band,
                            usize::from(self.ch_qmf_domain),
                        );

                        // Apply HBE transposition.
                        hbe.apply(
                            &mut low_band.real,
                            &mut low_band.imag,
                            frame_data.sbr_pitch_in_bins(),
                            num_ov_cols as u8,
                            kssm,
                        );

                        if flags.contains(SbrDecFlags::QUAD_RATE) {
                            let xover_qmf = hbe.x_over_band();

                            Self::copy_harmonic_spectrum(
                                xover_qmf,
                                &mut low_band.real,
                                &mut low_band.imag,
                                num_cols,
                                num_ov_cols,
                                kssm,
                            );
                        }
                    }
                }
            }

            let first_slot_offs = usize::from(frame_data.frame_info.borders()[0]);
            let num_freq_bands_noise =
                usize::from(header_data.freq_band_data().num_freq_bands_noise());

            if is_usac_harmonic_sbr
                && frame_data.sbr_patching_mode() == SbrPatchingMode::Hbe
                && !is_usac_hbe_lp
            {
                if let Some(hbe) = &self.hbe {
                    let mut low_band = QmfDomainQmfSlotsMut::default();
                    qd.get_pointer_array_for_qmf_slots_mut(
                        &mut low_band,
                        usize::from(self.ch_qmf_domain),
                    );

                    self.lpp_trans.apply(
                        lpp_trans_settings,
                        &mut low_band.real,
                        &mut low_band.imag,
                        frame_data.sbr_invf_mode(),
                        &prev_frame_data.sbr_invf_mode,
                        usize::from(hbe.start_band()),
                        usize::from(hbe.stop_band()),
                        num_freq_bands_noise,
                        0,
                        usize::from(header_data.time_step()),
                        first_slot_offs,
                        last_slot_offs,
                        true,
                        false,
                    );
                }
            } else {
                if is_usac_harmonic_sbr {
                    // Save states from previous frame in hbe lpc filterstate buffer.

                    let mut low_band = QmfDomainQmfSlots::default();
                    qd.get_pointer_array_for_qmf_slots(
                        &mut low_band,
                        usize::from(self.ch_qmf_domain),
                    );

                    self.lpp_trans.save_hbe_lpc_filter_states(
                        &low_band.real[num_cols - LPC_ORDER..num_cols + num_ov_cols],
                        &low_band.imag[num_cols - LPC_ORDER..num_cols + num_ov_cols],
                        LPC_ORDER + num_ov_cols,
                        SBRDEC_MAX_SUBBAND_SUBSAMPLES,
                    );
                }

                {
                    let mut low_band = QmfDomainQmfSlotsMut::default();
                    qd.get_pointer_array_for_qmf_slots_mut(
                        &mut low_band,
                        usize::from(self.ch_qmf_domain),
                    );

                    let start_patching = usize::from(lpp_trans_settings.lb_start_patching());
                    let stop_patching = usize::from(lpp_trans_settings.lb_stop_patching());
                    let v_k_master0 =
                        usize::from(header_data.freq_band_data().master_band_table()[0]);

                    self.lpp_trans.apply(
                        lpp_trans_settings,
                        &mut low_band.real,
                        &mut low_band.imag,
                        frame_data.sbr_invf_mode(),
                        &prev_frame_data.sbr_invf_mode,
                        start_patching,
                        stop_patching,
                        num_freq_bands_noise,
                        v_k_master0,
                        usize::from(header_data.time_step()),
                        first_slot_offs,
                        last_slot_offs,
                        false,
                        header_data.bitstream_info().preprocessing(),
                    );
                }
            }

            // Adjust envelope of current frame.

            if frame_data.sbr_patching_mode() != self.patching_mode && !is_usac_hbe_lp {
                let (is_b41sbr, xover_qmf) = if let Some(hbe) = &self.hbe {
                    let is_b41sbr = hbe.quad_rate();
                    let xover_qmf = if is_usac_harmonic_sbr
                        && frame_data.sbr_patching_mode() == SbrPatchingMode::Hbe
                    {
                        Some(*hbe.x_over_band())
                    } else {
                        None
                    };

                    (is_b41sbr, xover_qmf)
                } else {
                    (false, None)
                };

                let num_of_patches = usize::from(lpp_trans_settings.num_of_patches());
                let num_freq_bands_sbr =
                    usize::from(header_data.freq_band_data().num_freq_bands_sbr(LO));
                let limiter_bands = usize::from(header_data.bitstream().limiter_bands());

                let _ = self.calc_envelope.reset_limiter_bands(
                    Some(lpp_trans_settings.patch_param()),
                    header_data.freq_band_data().freq_band_table_low(),
                    num_freq_bands_sbr,
                    num_of_patches,
                    limiter_bands,
                    frame_data.sbr_patching_mode(),
                    &xover_qmf,
                    is_b41sbr,
                );

                self.patching_mode = frame_data.sbr_patching_mode();
            }

            let mut low_band = QmfDomainQmfSlotsMut::default();
            qd.get_pointer_array_for_qmf_slots_mut(&mut low_band, usize::from(self.ch_qmf_domain));

            self.calc_envelope.apply(
                header_data,
                frame_data,
                if let Some(pvc_data) = &mut self.pvc_data {
                    Some(pvc_data)
                } else {
                    None
                },
                &mut low_band.real,
                &mut low_band.imag,
                flags,
                frame_data.frame_error_flag() || prev_frame_data.frame_error_flag,
            );
        }

        if !is_usac_harmonic_sbr || is_usac_hbe_lp {
            let cplx_output = self.lpp_trans.lpc_filter_states_leg_sbr();

            qd.qmf_data_to_lpc_states(
                cplx_output,
                flags.contains(SbrDecFlags::SYNTAX_USAC),
                LPC_ORDER,
                usize::from(self.ch_qmf_domain),
            );
        }

        // Save current frame status.
        self.apply_sbr_proc_old = apply_processing;
    }

    /// Resets SBR decoder instance (for specific channel).
    ///
    /// # Parameters
    ///
    /// - `qd`: QMF domain (`QmfDomain`) instance.
    /// - `header_data`: SBR header data (`HeaderData`) instance.
    /// - `prev_frame_data`: SBR previous frame data (`PrevFrameData`) instance.
    /// - `patching_mode`: SBR transposer patching mode.
    /// - `num_overlap_ts`: Number of overlap time slots.
    /// - `flags`: SBR decoder flags.
    /// - `apply_sbr_proc`: Indicates whether sbr processing is applied.
    ///
    /// # Return
    ///
    /// Returns `Result<(), SbrError>`.
    #[expect(clippy::too_many_arguments)]
    pub fn reset(
        &mut self,
        qd: &mut QmfDomain,
        header_data: &HeaderData,
        prev_frame_data: &PrevFrameData,
        patching_mode: SbrPatchingMode,
        num_overlap_ts: usize,
        flags: SbrDecFlags,
        apply_sbr_proc: bool,
    ) -> Result<(), SbrError> {
        let (new_lsb, old_lsb) = if let Some(ana_fb) = qd.get_filter_bank(
            usize::from(self.ch_qmf_domain),
            QmfDomainSelect::QmfDomainIn,
        ) {
            let num_channels_in = ana_fb.num_subbands();

            let lsb_new = if !apply_sbr_proc {
                num_channels_in
            } else {
                header_data.freq_band_data().start_subband
            } as usize;

            let lsb_old = if !self.apply_sbr_proc_old {
                num_channels_in
            } else {
                ana_fb.num_lf_subbands()
            } as usize;

            (lsb_new, lsb_old)
        } else {
            (0, 0)
        };

        self.calc_envelope.reset();

        // Change lsb and usb.

        // Synthesis.
        let (lsb, usb) = if let Some(syn_fb) = qd.get_filter_bank_mut(
            usize::from(self.ch_qmf_domain),
            QmfDomainSelect::QmfDomainOut,
        ) {
            let num_channels_out = syn_fb.num_subbands();

            let lsb = num_channels_out.min(header_data.freq_band_data().start_subband);
            let usb = num_channels_out.min(header_data.freq_band_data().end_subband);

            syn_fb.set_num_lf_subbands(lsb);
            syn_fb.set_num_hf_subbands(usb);

            (lsb, usb)
        } else {
            (0, 0)
        };

        // Analysis.
        if let Some(ana_fb) = qd.get_filter_bank_mut(
            usize::from(self.ch_qmf_domain),
            QmfDomainSelect::QmfDomainIn,
        ) {
            ana_fb.set_num_lf_subbands(lsb);
            ana_fb.set_num_hf_subbands(usb);
        }

        // The following initialization of spectral data in the overlap buffer
        // is required for dynamic x-over or a change of the start-freq for 2 reasons:
        //
        // 1. If the lowband gets _wider_, unadjusted data would remain.
        //
        // 2. If the lowband becomes _smaller_, the highest bands of the old lowband
        // must be cleared because the whitening would be affected.

        let start_slot = usize::from(
            header_data.time_step()
                * (prev_frame_data
                    .stop_pos
                    .saturating_sub(header_data.number_time_slots())),
        );

        let mut ov_qmf_slots = QmfDomainOvQmfSlotsMut::default();
        qd.get_pointer_array_for_ov_qmf_slots_mut(
            &mut ov_qmf_slots,
            usize::from(self.ch_qmf_domain),
        );

        // In case of USAC we don't want to zero out the memory, as this can lead to
        // holes in the spectrum; The special handling shall only be applied for USAC not for
        // MPEG-4 SBR, in this case setting zero remains.
        if !flags.contains(SbrDecFlags::SYNTAX_USAC) {
            let start_band = old_lsb;
            let stop_band = new_lsb;
            let size = stop_band.saturating_sub(start_band);

            // Keep already adjusted data in the x-over-area.
            for (real, imag) in izip!(
                ov_qmf_slots.real[start_slot..].iter_mut(),
                ov_qmf_slots.imag[start_slot..].iter_mut()
            )
            .take(num_overlap_ts - start_slot)
            {
                real[start_band..start_band + size].fill(0.0);
                imag[start_band..start_band + size].fill(0.0);
            }

            // Reset LPC filter states.
            let start_band = old_lsb.min(new_lsb);
            let stop_band = old_lsb.max(new_lsb);
            let size = (stop_band - start_band).max(0);

            let leg_sbr = self.lpp_trans.lpc_filter_states_leg_sbr();
            leg_sbr[0][start_band..start_band + size].fill(Complex::new(0.0, 0.0));
            leg_sbr[1][start_band..start_band + size].fill(Complex::new(0.0, 0.0));
        }

        self.are_states_saved = false;

        if flags.contains(SbrDecFlags::USAC_HARMONICSBR) && apply_sbr_proc {
            if let Some(hbe) = &mut self.hbe {
                let freq_band_table = [
                    header_data.freq_band_data().freq_band_table_low(),
                    header_data.freq_band_data().freq_band_table_high(),
                ];

                let num_freq_bands_sbr = [
                    header_data.freq_band_data().num_freq_bands_sbr(LO),
                    header_data.freq_band_data().num_freq_bands_sbr(HI),
                ];

                hbe.reinit(&freq_band_table, &num_freq_bands_sbr)?;

                let num_colums = usize::from(hbe.no_cols());
                let num_channels = usize::from(hbe.no_channels());

                {
                    // Copy saved states from previous frame to legacy SBR lpc filterstate
                    // buffer.
                    let hbe_buffer = &hbe.hbe_buffer;
                    let lpc_legacy_sbr = self.lpp_trans.lpc_filter_states_leg_sbr();

                    for (legacy_data, hbe_real, hbe_imag) in izip!(
                        lpc_legacy_sbr[..LPC_ORDER + num_overlap_ts].iter_mut(),
                        hbe_buffer.real()[num_colums - LPC_ORDER - num_overlap_ts..].iter(),
                        hbe_buffer.imag()[num_colums - LPC_ORDER - num_overlap_ts..].iter()
                    )
                    .take(LPC_ORDER + num_overlap_ts)
                    {
                        for (leg_data, hbe_re, hbe_im) in
                            izip!(legacy_data.iter_mut(), hbe_real.iter(), hbe_imag.iter())
                                .take(num_channels)
                        {
                            leg_data.re = *hbe_re;
                            leg_data.im = *hbe_im;
                        }
                    }
                }

                self.are_states_saved = true;

                {
                    {
                        let mut low_band_real: [&mut [f32]; MAX_OV_COLS + LPC_ORDER] =
                            array::from_fn(|_| [].as_mut_slice());
                        let mut low_band_imag: [&mut [f32]; MAX_OV_COLS + LPC_ORDER] =
                            array::from_fn(|_| [].as_mut_slice());

                        let (lpc_hbe_real, lpc_hbe_imag) =
                            self.lpp_trans.lpc_filter_states_hbe_mut();

                        for (lb_real, lb_imag, hbe_real, hbe_imag) in izip!(
                            low_band_real.iter_mut(),
                            low_band_imag.iter_mut(),
                            lpc_hbe_real.iter_mut(),
                            lpc_hbe_imag.iter_mut()
                        )
                        .take(LPC_ORDER + num_overlap_ts)
                        {
                            *lb_real = hbe_real;
                            *lb_imag = hbe_imag;
                        }

                        if flags.contains(SbrDecFlags::QUAD_RATE) {
                            if patching_mode == SbrPatchingMode::Hbe {
                                hbe.apply(
                                    &mut low_band_real[..LPC_ORDER + num_overlap_ts],
                                    &mut low_band_imag[..LPC_ORDER + num_overlap_ts],
                                    prev_frame_data.sbr_pitch_in_bins,
                                    num_overlap_ts as u8,
                                    KeepStatesSyncedMode::OutDiff,
                                );

                                let xover_qmf = hbe.x_over_band();
                                Self::copy_harmonic_spectrum(
                                    xover_qmf,
                                    &mut low_band_real,
                                    &mut low_band_imag,
                                    num_colums,
                                    num_overlap_ts,
                                    KeepStatesSyncedMode::OutDiff,
                                );
                            }
                        } else {
                            if patching_mode == SbrPatchingMode::Hbe {
                                hbe.apply(
                                    &mut low_band_real[..LPC_ORDER + num_overlap_ts],
                                    &mut low_band_imag[..LPC_ORDER + num_overlap_ts],
                                    0,
                                    num_overlap_ts as u8,
                                    KeepStatesSyncedMode::NoOut,
                                );
                            }

                            hbe.apply(
                                &mut low_band_real[..LPC_ORDER + num_overlap_ts],
                                &mut low_band_imag[..LPC_ORDER + num_overlap_ts],
                                prev_frame_data.sbr_pitch_in_bins,
                                num_overlap_ts as u8,
                                KeepStatesSyncedMode::OutDiff,
                            );
                        }
                    }

                    if patching_mode == SbrPatchingMode::Hbe {
                        // Store the unmodified qmf slot values for upper part of spectrum
                        // (required for LPC filtering) Required if next frame is a HBE
                        // frame.
                        let (lpc_hbe_real, lpc_hbe_imag) = self.lpp_trans.lpc_filter_states_hbe();

                        for (ov_qmf_real, ov_qmf_imag, hbe_real, hbe_imag) in izip!(
                            ov_qmf_slots.real[start_slot..].iter_mut(),
                            ov_qmf_slots.imag[start_slot..].iter_mut(),
                            lpc_hbe_real[start_slot + LPC_ORDER..].iter(),
                            lpc_hbe_imag[start_slot + LPC_ORDER..].iter()
                        )
                        .take(num_overlap_ts - start_slot)
                        {
                            ov_qmf_real[..SBRDEC_MAX_SUBBAND_SUBSAMPLES].copy_from_slice(hbe_real);
                            ov_qmf_imag[..SBRDEC_MAX_SUBBAND_SUBSAMPLES].copy_from_slice(hbe_imag);
                        }

                        // Store the unmodified qmf slot values for upper part of
                        // the spectrum (required for LPC filtering). Required if next
                        // frame is a HBE frame.

                        let hbe_buffer = &hbe.hbe_buffer;
                        for (ov_qmf_real, ov_qmf_imag, hbe_real, hbe_imag) in izip!(
                            ov_qmf_slots.real[start_slot..].iter_mut(),
                            ov_qmf_slots.imag[start_slot..].iter_mut(),
                            hbe_buffer.real()[num_colums - num_overlap_ts + start_slot..].iter(),
                            hbe_buffer.imag()[num_colums - num_overlap_ts + start_slot..].iter()
                        )
                        .take(num_overlap_ts - start_slot)
                        {
                            ov_qmf_real[..new_lsb].copy_from_slice(&hbe_real[..new_lsb]);
                            ov_qmf_imag[..new_lsb].copy_from_slice(&hbe_imag[..new_lsb]);
                        }
                    }
                }
            }
        }

        if patching_mode == SbrPatchingMode::Lpp && flags.contains(SbrDecFlags::SYNTAX_USAC) {
            // Get missing states between old and new x_over from LegSBR filterstates
            // buffer, in case of legacy SBR we leave these values zeroed out.
            let len = new_lsb.saturating_sub(old_lsb);

            let lpc_legacy_sbr = self.lpp_trans.lpc_filter_states_leg_sbr();
            for (ov_qmf_real, ov_qmf_imag, legacy_sbr) in izip!(
                ov_qmf_slots.real[start_slot..].iter_mut(),
                ov_qmf_slots.imag[start_slot..].iter_mut(),
                lpc_legacy_sbr[LPC_ORDER + start_slot..].iter()
            )
            .take(num_overlap_ts - start_slot)
            {
                for (leg_sbr, ov_re, ov_im) in izip!(
                    legacy_sbr[old_lsb..].iter(),
                    ov_qmf_real[old_lsb..].iter_mut(),
                    ov_qmf_imag[old_lsb..].iter_mut()
                )
                .take(len)
                {
                    *ov_re = leg_sbr.re;
                    *ov_im = leg_sbr.im;
                }
            }
        }

        self.patching_mode = patching_mode;

        Ok(())
    }

    /// Copies harmonic spectrum.
    ///
    /// # Parameters
    ///
    /// - `xover_qmf`: Cross-over frequency table.
    /// - `qmf_real`: Real part of qmf spectrum.
    /// - `qmf_imag`: Imaginary part of qmf spectrum.
    /// - `num_ts`: Number of qmf time slots.
    /// - `num_ov_ts`: Number of overlap time slots.
    /// - `kssm`: Mode to describe qmf transposer behaviour.
    fn copy_harmonic_spectrum(
        xover_qmf: &[u8; HBE_MAX_NUM_PATCHES as usize],
        qmf_real: &mut [&mut [f32]],
        qmf_imag: &mut [&mut [f32]],
        num_ts: usize,
        num_ov_ts: usize,
        kssm: KeepStatesSyncedMode,
    ) {
        let num_patches = xover_qmf
            .iter()
            .take(MAX_NUM_PATCHES)
            .skip(1)
            .fold(0_usize, |acc, &data| acc + if data != 0 { 1 } else { 0 });

        let patch_start = (HBE_MAX_STRETCH - 1) as usize;
        if patch_start < num_patches {
            let source_bands =
                xover_qmf[HBE_MAX_STRETCH as usize - 1] - xover_qmf[HBE_MAX_STRETCH as usize - 2];

            for xover in xover_qmf[patch_start..]
                .windows(2)
                .take(num_patches - patch_start)
            {
                let mut target = xover[0];
                let mut patch_bands = xover[1] as i16 - xover[0] as i16;

                'patch_band_loop: loop {
                    if patch_bands > 0 {
                        let mut num_bands = source_bands;
                        let mut start_band =
                            usize::from(xover_qmf[HBE_MAX_STRETCH as usize - 1]) - 1;

                        if target + num_bands >= xover[1] {
                            num_bands = xover[1] - target;
                        }

                        if (((target + num_bands - 1) % 2)
                            + ((xover_qmf[HBE_MAX_STRETCH as usize - 1] - 1) % 2))
                            % 2
                            != 0
                        {
                            if num_bands == source_bands {
                                num_bands -= 1;
                            } else {
                                start_band -= 1;
                            }
                        }

                        let (start_slot, end_slot) = if kssm == KeepStatesSyncedMode::OutDiff {
                            (0, LPC_ORDER + num_ov_ts)
                        } else {
                            let ss = if kssm == KeepStatesSyncedMode::Normal {
                                num_ts - LPC_ORDER
                            } else {
                                num_ov_ts
                            };

                            (ss, num_ts + num_ov_ts)
                        };

                        let max_num_bands = usize::from(
                            ((xover[1].min(64_u8)).saturating_sub(target)).min(num_bands),
                        );

                        for (real_slot, imag_slot) in izip!(
                            qmf_real[start_slot..end_slot].iter_mut(),
                            qmf_imag[start_slot..end_slot].iter_mut()
                        )
                        .take(end_slot - start_slot)
                        {
                            real_slot.copy_within(
                                (start_band - (max_num_bands - 1))..=start_band,
                                target as usize,
                            );
                            imag_slot.copy_within(
                                (start_band - (max_num_bands - 1))..=start_band,
                                target as usize,
                            );
                        }

                        target += num_bands;
                        patch_bands -= num_bands as i16;
                    } else {
                        break 'patch_band_loop;
                    }
                }
            }
        }
    }

    /// Initializes SBR decoder and allocates optional internal sub-components such as
    /// `HbeTransposer` and `PvcDecoder`.
    ///
    /// # Parameters
    ///
    /// - `flags`: SBR decoder flags.
    /// - `codec_frame_size`: Core codec frame size.
    /// - `num_columns`: Number of qmf slots.
    pub fn create(&mut self, flags: SbrDecFlags, codec_frame_size: u16, num_columns: u8) {
        self.codec_frame_size = codec_frame_size;

        // Disable HBE processing. Once a MPEG-4 SBR Enhancements element is available in the
        // bitstream the HBE processing is enabled.

        if !flags.contains(SbrDecFlags::SYNTAX_USAC)
            && flags.contains(SbrDecFlags::USAC_HARMONICSBR)
        {
            self.esbr_mode = SbrDecEsbrMode::Off;
        }

        // Init envelope calculator.
        self.calc_envelope.init();

        if flags.contains(SbrDecFlags::USAC_HARMONICSBR) {
            let is_sbr41 = flags.contains(SbrDecFlags::QUAD_RATE);

            if self.hbe.is_none() {
                self.hbe = Some(Box::new(HbeTransposer::new(
                    codec_frame_size,
                    num_columns,
                    is_sbr41,
                )));
            }
        }

        if flags.contains(SbrDecFlags::SYNTAX_USAC) && self.pvc_data.is_none() {
            self.pvc_data = Some(Box::new(PvcDecoder::new()));
        }
    }

    /// Deallocates optional internal sub-components if they were allocated previously.
    pub fn delete(&mut self) {
        if self.hbe.is_some() {
            self.hbe = None;
        }

        if self.pvc_data.is_some() {
            self.pvc_data = None;
        }
    }
}
