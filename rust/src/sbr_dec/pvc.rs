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
//! Predictive vector coding (PVC) decoding process

// Modules
mod consts;
mod tables;

// Imports
use consts::*;
use tables::*;

/// PvcDecoder structure
///
/// In order to improve the subjective quality of the eSBR tool,
/// in particular for speech content at low bitrates,
/// predictive vector coding (PVC) is added to the eSBR tool
/// (ISO/IEC 23003-3:2020(E) 7.5.6).
///
/// Generally, for speech signals, there is a relatively
/// high correlation between the spectral envelopes of
/// low frequency bands and high frequency bands.
/// In the PVC scheme, this is exploited by the
/// prediction of the spectral envelope in high frequency bands
/// from the spectral envelope in low frequency bands,
/// where the coefficient matrices for the prediction
/// are coded by means of vector quantization.
/// The analysis and synthesis QMF banks and HF generator
/// remain unchanged, but the HF envelope adjuster
/// is modified to process the envelopes generated by the PVC decoder.
/// The indices of the coefficient matrices for the prediction,
/// pvcID(t) , t=0,1,2,...,15 are transmitted in the bitstream.
#[derive(Debug, Default, Copy, Clone)]
pub struct PvcDecoder {
    /// Xover frequency of last frame.
    kx: u8,
    /// PVC mode of last frame.
    mode: Mode,
    /// Start SBR time slot of last PVC frame.
    border0: u8,
    /// Ring buffer index to current Esg time slot.
    esg_slot_index: u8,
    /// Esg(ksg,t) of current and 15 previous time slots.
    esg: [[f32; NBLOW]; NS_MAX as usize],
    /// Predicted Energy in linear domain
    pred_esg: [[f32; NBHIGH_MAX]; NTIMESLOT],
    /// Number of grouped QMF subbands in the SBR range.
    nb_high: u8,
    /// Number of QMF subbands for a grouped QMF subband below the SBR range.
    lbw: u8,
    /// Number of QMF subbands for a grouped QMF subband in the SBR range.
    hbw: u8,
    /// Number of time slots for time-domain smoothing of Esg(ksg,t).
    ns: Option<Ns>,
    /// Number of QMF subband samples per time slot (2 or 4).
    rate: Option<Rate>,
    /// Number of past Esg(ksg,t) which are available for smoothing filter.
    past_esg_slots_avail: u8,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd)]
pub enum Mode {
    #[default]
    None = 0,
    Mode1 = 1,
    Mode2 = 2,
    Reserved = 3,
}

impl From<usize> for Mode {
    fn from(val: usize) -> Self {
        match val {
            x if x == Mode::None as usize => Mode::None,
            x if x == Mode::Mode1 as usize => Mode::Mode1,
            x if x == Mode::Mode2 as usize => Mode::Mode2,
            x if x == Mode::Reserved as usize => Mode::Reserved,
            _ => panic!("invalid mode"),
        }
    }
}

impl From<u8> for Mode {
    fn from(val: u8) -> Self {
        Into::<usize>::into(val).into()
    }
}

impl From<Mode> for usize {
    fn from(val: Mode) -> Self {
        val as usize
    }
}

#[derive(Debug, Clone, Copy)]
pub(super) enum Ns {
    Ns16 = 16,
    Ns12 = 12,
    Ns4 = 4,
    Ns3 = 3,
}

impl From<usize> for Ns {
    fn from(val: usize) -> Self {
        match val {
            x if x == Ns::Ns16 as usize => Ns::Ns16,
            x if x == Ns::Ns12 as usize => Ns::Ns12,
            x if x == Ns::Ns4 as usize => Ns::Ns4,
            x if x == Ns::Ns3 as usize => Ns::Ns3,
            _ => panic!("invalid ns"),
        }
    }
}

impl From<u8> for Ns {
    fn from(val: u8) -> Self {
        Into::<usize>::into(val).into()
    }
}

impl From<Ns> for usize {
    fn from(val: Ns) -> Self {
        val as usize
    }
}

#[derive(Debug, Clone, Copy)]
pub(super) enum Rate {
    Rate2 = 2,
    Rate4 = 4,
}

impl From<usize> for Rate {
    fn from(val: usize) -> Self {
        match val {
            x if x == Rate::Rate2 as usize => Rate::Rate2,
            x if x == Rate::Rate4 as usize => Rate::Rate4,
            _ => panic!("invalid rate"),
        }
    }
}

impl From<u8> for Rate {
    fn from(val: u8) -> Self {
        Into::<usize>::into(val).into()
    }
}

impl From<Rate> for usize {
    fn from(val: Rate) -> Self {
        val as usize
    }
}

impl From<Rate> for u8 {
    fn from(val: Rate) -> Self {
        val as u8
    }
}

impl PvcDecoder {
    /// Create a new PVC decoder
    pub(super) fn new() -> Self {
        PvcDecoder::default()
    }

    /// Get the PVC mode of the last frame.
    pub fn mode(&self) -> Mode {
        self.mode
    }

    /// Decode the PVC frame.
    /// (feed the PVC decoder with the QMF buffer and the PVC frame information)
    ///
    /// # Arguments
    /// * `qmf_buffer_real` - The real part of the QMF buffer.
    /// * `qmf_buffer_imag` - The imaginary part of the QMF buffer.
    /// * `id` - The indices of the coefficient matrices for the prediction, pvcID(t),
    ///   t=0,1,2,...,15 are transmitted in the bitstream.
    /// * `mode` - The PVC mode.
    /// * `ns` - The number of time slots for time-domain smoothing of Esg(ksg,t).
    /// * `rate` - The number of QMF subband samples per time slot (2 or 4).
    /// * `kx` - Xover frequency.
    /// * `border0` - Start SBR time slot.
    #[expect(clippy::too_many_arguments)]
    pub(super) fn decode(
        &mut self,
        qmf_buffer_real: &[&[f32]],
        qmf_buffer_imag: &[&[f32]],
        id: &[u8; NTIMESLOT],
        mode: Mode,
        ns: Option<Ns>,
        rate: Rate,
        kx: u8,
        border0: u8,
    ) {
        self.update(mode, ns, rate, kx, border0);

        if self.mode != Mode::None {
            let rate: usize = self.rate.unwrap().into();

            let border0: usize = self.border0.into();
            for (t, (real, imag)) in qmf_buffer_real[border0 * rate..]
                .chunks_exact(rate)
                .zip(qmf_buffer_imag[border0 * rate..].chunks_exact(rate))
                .enumerate()
            {
                let ts = border0 + t;
                self.decode_time_slot(real, imag, ts, id[ts]);
            }
        }
    }

    /// Expand the predicted ESG. (result of the PVC decoding process)
    /// # Arguments
    /// * `time_slot` - The time slot.
    /// * `output` - The output buffer.
    pub(super) fn expand_pred_esg(&mut self, time_slot: usize, output: &mut [f32]) {
        let pred_esg = self.pred_esg[time_slot];

        let mut out_it = output.iter_mut();

        let nb_high: usize = self.nb_high.into();
        for pr in pred_esg.iter().take(nb_high) {
            for _ in 0..self.hbw {
                *(out_it.next().unwrap()) = *pr;
            }
        }
        for out in out_it {
            *out = pred_esg[nb_high - 1];
        }
    }

    fn update(&mut self, mode: Mode, ns: Option<Ns>, rate: Rate, kx: u8, border0: u8) {
        let mut mode = mode;
        let urate: u8 = rate.into();
        match ns {
            Some(ns) => match ns {
                Ns::Ns16 | Ns::Ns12 | Ns::Ns4 | Ns::Ns3 => {
                    match mode {
                        Mode::Mode1 => {
                            self.nb_high = 8;
                            self.hbw = 8 / urate;
                            self.lbw = 8 / urate;
                        }
                        Mode::Mode2 => {
                            self.nb_high = 6;
                            self.hbw = 12 / urate;
                            self.lbw = 8 / urate;
                        }
                        Mode::None => {}
                        Mode::Reserved => {}
                    }

                    if mode != Mode::None {
                        if self.mode == Mode::None || self.kx != kx {
                            self.past_esg_slots_avail = 0;
                        } else {
                            self.past_esg_slots_avail = NS_MAX - self.border0;
                        }
                        self.kx = kx;
                        self.border0 = border0;
                        self.ns = Some(ns);
                        self.rate = Some(rate);
                    }
                }
            },
            None => {
                mode = Mode::None;
            }
        }
        self.mode = mode;
    }

    fn decode_time_slot(
        &mut self,
        qmf_slot_real: &[&[f32]],
        qmf_slot_imag: &[&[f32]],
        time_slot_number: usize,
        id: u8,
    ) {
        let mut e = [0.0; NBLOW];

        // Subband grouping in QMF subbands below SBR range.
        // Within one timeslot ( i = [0...(RATE-1)] QMF subsamples) calculate energy
        // E(ib,t) and group them to Esg(ksg,t). Then transfer values to logarithmical
        // domain and store them for time domain smoothing. (7.5.6.3 Subband grouping in
        // QMF subbands below SBR range)

        let kx_i: isize = self.kx.into();
        let ilbw: isize = self.lbw.into();
        let esg_slot_index: usize = self.esg_slot_index.into();
        let ksg_start = ((-kx_i + NBLOW as isize * ilbw + ilbw - 1) / ilbw).max(0);
        let ksg_start_u = ksg_start as usize;
        for ksg in 0..ksg_start_u {
            self.esg[esg_slot_index][ksg] = -10.0; // 10*log10(0.1)
        }

        for i in 0..self.rate.unwrap().into() {
            let qmf_r = qmf_slot_real[i];
            let qmf_i = qmf_slot_imag[i];
            let ikx: isize = self.kx.into();
            let ilbw: isize = self.lbw.into();
            let band: isize = ikx + ((ksg_start - NBLOW as isize) * ilbw);

            if band >= 0 {
                let mut band: usize = band.try_into().unwrap();
                for e_ksg in e.iter_mut().take(NBLOW).skip(ksg_start.try_into().unwrap()) {
                    let mut e_tmp = 0.0;
                    for _ in 0..self.lbw {
                        e_tmp += qmf_r[band] * qmf_r[band] + qmf_i[band] * qmf_i[band];
                        band += 1;
                    }
                    // The division by 8 == (RATE*lbw) is required algorithmically.
                    *e_ksg += e_tmp / 8.0;
                }
            }
        }

        {
            let esg_slot_index: usize = self.esg_slot_index.into();
            for (ksg, e_ksg) in e
                .iter()
                .enumerate()
                .take(NBLOW)
                .skip(ksg_start.try_into().unwrap())
            {
                self.esg[esg_slot_index][ksg] = e_ksg.max(1.0 / 10.0).log10() * 10.0;
            }
        }
        let mut e = [0.0; NBLOW];

        let smooth = self.smoothing_window_table();
        let mut idx: usize = self.esg_slot_index.into();
        let ns = self.ns.unwrap().into();
        let avail: usize = self.past_esg_slots_avail.into();
        let mut it = smooth.iter();
        let avail = avail.min(ns);
        for _ in 0..avail {
            let sc_coeff = it.next().unwrap();
            let esg_idx: &[f32; NBLOW] = &self.esg[idx];
            e[0] += esg_idx[0] * sc_coeff;
            e[1] += esg_idx[1] * sc_coeff;
            e[2] += esg_idx[2] * sc_coeff;
            idx = ((idx as isize - 1) & (NS_MAX as isize - 1)) as usize;
        }
        // If past Esg values are not available use the ones from the last valid
        // slot.
        for _ in avail..ns {
            let sc_coeff = it.next().unwrap();
            let esg_idx: &[f32; NBLOW] = &self.esg[idx];
            e[0] += esg_idx[0] * sc_coeff;
            e[1] += esg_idx[1] * sc_coeff;
            e[2] += esg_idx[2] * sc_coeff;
        }

        for ksg in 0..self.nb_high {
            let uksg: usize = ksg.into();
            let tab1 = self.prediction_coefficient_table1(id, uksg);

            let tab2 = self.prediction_coefficient_table2(id);

            let scaling_coef = self.scaling_coefficient_table();
            // Residual part.
            let mut accu = tab2[uksg] as f32 * scaling_coef[3];
            // Linear combination of lower grouped energies part.
            for kb in 0..NBLOW {
                let pred_coeff = tab1[kb] as f32;
                accu += e[kb] * pred_coeff * scaling_coef[kb];
            }
            // Convert back to linear domain.
            self.pred_esg[time_slot_number][uksg] = 10.0f32.powf(accu / 10.0);
            // self.pred_esg[time_slot_number][uksg] = (accu * (10.0f32.ln() / 10.0)).exp();
        }

        self.past_esg_slots_avail = (self.past_esg_slots_avail + 1).min(NS_MAX - 1);
        self.esg_slot_index = (self.esg_slot_index + 1) & (NS_MAX - 1);
    }

    fn prediction_coefficient_table1(&self, id: u8, ksg: usize) -> &[i8] {
        let tab1_dp = match self.mode {
            Mode::Mode1 => &G_A_TAB1_DP_MODE1,
            Mode::Mode2 => &G_A_TAB1_DP_MODE2,
            _ => panic!("Invalid mode"),
        };

        let tab1_id = if id < tab1_dp[0] {
            0
        } else if id < tab1_dp[1] {
            1
        } else {
            2
        };

        match self.mode {
            Mode::Mode1 => G_3A_TAB1_MODE1[tab1_id][ksg].as_slice(),
            Mode::Mode2 => G_3A_TAB1_MODE2[tab1_id][ksg].as_slice(),
            _ => panic!("Invalid mode"),
        }
    }

    fn prediction_coefficient_table2(&self, id: u8) -> &[i8] {
        match self.mode {
            Mode::Mode1 => G_2A_TAB2_MODE1[id as usize].as_slice(),
            Mode::Mode2 => G_2A_TAB2_MODE2[id as usize].as_slice(),
            _ => panic!("Invalid mode"),
        }
    }

    fn scaling_coefficient_table(&self) -> &[f32] {
        match self.mode {
            Mode::Mode1 => G_A_SCALING_COEF_MODE1.as_slice(),
            Mode::Mode2 => G_A_SCALING_COEF_MODE2.as_slice(),
            _ => panic!("Invalid mode"),
        }
    }

    fn smoothing_window_table(&self) -> &[f32] {
        match self.ns.unwrap() {
            Ns::Ns16 => SC_16.as_slice(),
            Ns::Ns12 => SC_12.as_slice(),
            Ns::Ns4 => SC_4.as_slice(),
            Ns::Ns3 => SC_3.as_slice(),
        }
    }
}
