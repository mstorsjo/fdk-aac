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
//! Hybrid filterbank

use crate::common::fft;

use itertools::izip;
use num_complex::Complex;

// Prototype length of the filter bank.
const PROTO_LEN: usize = 13;

const MAX_QMF_BANDS_TO_HYBRID: usize = 3;
const MAX_NUM_QMF_BANDS: usize = 64;
pub(in super::super) const NUM_HYBRID_DATA_BANDS: usize = 71;

/// Structure describing the hybrid analysis filter bank.
#[derive(Debug, Default)]
#[repr(C)]
pub struct HybridAnalysis {
    /// Position to write incoming data into ring buffer.
    lf_pos: usize,
    /// Delay line positioning.
    hf_pos: usize,
    /// Number of QMF bands.
    pub num_bands: u8,

    /// Flag that signalizes treatment of HF bands regarding delay compensation.
    pub hf_mode: u8,
    /// Pointer to filter setup.
    pub setup: &'static HybridSetup,

    /// LF states buffer.
    lf_memory: [[f32; 2 * PROTO_LEN]; 3],
    /// Optional HF states buffer, dynamically allocated.
    hf_memory: Option<Box<[[f32; 2 * (MAX_NUM_QMF_BANDS - MAX_QMF_BANDS_TO_HYBRID)]; 6]>>,
}

impl Default for &'static HybridSetup {
    fn default() -> &'static HybridSetup {
        &SETUP_3_10
    }
}

/// Structure describing the hybrid synthesis filter bank.
#[derive(Debug, Default, Clone, Copy)]
#[repr(C)]
pub struct HybridSynthesis {
    /// Number of QMF bands.
    pub num_bands: usize,
    /// Pointer to filter setup.
    pub setup: &'static HybridSetup,
}

/// Structure describing the hybrid filter bank setup.
#[derive(Debug)]
#[repr(C)]
pub struct HybridSetup {
    /// Number of QMF bands to be converted to hybrid.
    pub num_qmf_bands: u8,
    /// Number of hybrid bands generated by num_qmf_bands.
    pub num_hyb_bands: [u8; 3usize],
    /// Filter configuration of each QMF band.
    k_hybrid: [i8; 3usize],
    /// Prototype filter length.
    proto_len: u8,
    /// Delay caused by hybrid filter.
    filter_delay: u8,
}

// Structure instance describing the 3-to-10 hybrid filter bank setup.
pub static SETUP_3_10: HybridSetup = HybridSetup {
    num_qmf_bands: 3,
    num_hyb_bands: [6, 2, 2],
    k_hybrid: [-8, -2, 2],
    proto_len: 13,
    filter_delay: 6,
};

const HYB_FILTER_COEF8: [Complex<f32>; PROTO_LEN] = [
    Complex {
        re: 0.12500000000000,
        im: 0.00000000000000,
    },
    Complex {
        re: 0.10895967483521,
        im: -0.04513257741928,
    },
    Complex {
        re: -0.00527560291812,
        im: 0.00527560291812,
    },
    Complex {
        re: 0.06989827007055,
        im: -0.06989827007055,
    },
    Complex {
        re: -0.00868852436543,
        im: 0.02097595483065,
    },
    Complex {
        re: 0.02780621498823,
        im: -0.06713014096022,
    },
    Complex {
        re: 0.0,
        im: 0.04546865820885,
    },
    Complex {
        re: 0.0,
        im: 0.04546865820885,
    },
    Complex {
        re: 0.02780621498823,
        im: 0.06713014096022,
    },
    Complex {
        re: -0.00868852436543,
        im: -0.02097595483065,
    },
    Complex {
        re: 0.06989827007055,
        im: 0.06989827007055,
    },
    Complex {
        re: -0.00527560291812,
        im: -0.00527560291812,
    },
    Complex {
        re: 0.10895967483521,
        im: 0.04513257741928,
    },
];

const HYB_FILTER_COEF2: [f32; 3] = [0.01899487526049, -0.07293139167538, 0.30596630545168];

impl HybridAnalysis {
    /// Creates an instance of the hybrid analysis filter bank.
    ///
    /// # Parameters
    ///
    /// - `is_hf_enabled`: Method used to transport audio data.
    pub fn new(is_hf_enabled: bool) -> Self {
        let mut hybrid_analysis_instance = HybridAnalysis::default();
        hybrid_analysis_instance.allocate(is_hf_enabled);
        hybrid_analysis_instance
    }

    /// Allocates the optional high frequency states buffer.
    ///
    /// # Parameters
    ///
    /// - `is_hf_enabled`: Method used to transport audio data.
    pub fn allocate(&mut self, is_hf_enabled: bool) {
        if is_hf_enabled {
            self.hf_memory = Some(Box::new([[0.0_f32; 2 * 61]; 6]));
        }
    }

    /// Initializes and configures an hybrid analysis filter bank instance.
    ///
    /// # Parameters
    ///
    /// - `num_qmf_bands`:       Number of qmf bands to be processed.
    /// - `do_reset_states`:     Indicates whether the states buffers have to be cleared.
    pub fn init(&mut self, num_qmf_bands: u8, do_reset_states: bool) {
        self.setup = &SETUP_3_10;
        if do_reset_states {
            // Reset the buffer positions.
            self.lf_pos = usize::from(self.setup.proto_len - 1);
            self.hf_pos = 0;
        }
        self.num_bands = num_qmf_bands;
        self.hf_mode = 0;

        if do_reset_states {
            // Clear the LF states buffer.
            for row in &mut self.lf_memory {
                row.fill(0.0);
            }

            if let Some(hf_memory) = self.hf_memory.as_deref_mut() {
                if num_qmf_bands > self.setup.num_qmf_bands {
                    // Clear the HF states buffer.
                    for row in &mut hf_memory.iter_mut() {
                        row.fill(0.0);
                    }
                }
            }
        }
    }

    /// Applies hybrid analysis filtering on QMF input data.
    ///
    /// # Parameters
    ///
    /// - `qmf_real`:           QMF input data (real part).
    /// - `qmf_imag`:           QMF input data (imaginary part).
    /// - `hybrid`:             Hybrid output data (complex).
    ///
    /// # Return
    ///
    /// - 0 on success.
    /// - < 0 else.
    pub fn apply(
        &mut self,
        qmf_real: &[f32],
        qmf_imag: &[f32],
        hybrid: &mut [Complex<f32>],
    ) -> i32 {
        let mut err = 0;
        let mut hyb_offset = 0;

        // Number of QMF bands to be converted to hybrid.
        let num_qmf_bands_lf = usize::from(self.setup.num_qmf_bands);
        let write_index = self.lf_pos;
        let mut read_index = self.lf_pos;

        read_index = (read_index + 1) % usize::from(self.setup.proto_len);

        let mut buffer_lf_read_idx: [usize; PROTO_LEN] = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
        buffer_lf_read_idx.rotate_left(read_index);

        // LF buffer.
        for k in 0..num_qmf_bands_lf {
            // New input sample.
            self.lf_memory[k][write_index] = qmf_real[k];
            self.lf_memory[k][usize::from(self.setup.proto_len) + write_index] = qmf_imag[k];

            // Perform input sampling.
            err = Self::k_channel_filtering(
                &self.lf_memory[k][..2 * usize::from(self.setup.proto_len)],
                &buffer_lf_read_idx,
                &mut hybrid[hyb_offset..],
                self.setup.k_hybrid[k],
            );
            hyb_offset += usize::from(self.setup.num_hyb_bands[k]);
        }

        self.lf_pos = read_index;

        if usize::from(self.num_bands) > num_qmf_bands_lf {
            // HF buffer.
            if self.hf_mode != 0 {
                // If HF delay compensation was applied outside.
                for (real, imag, hyb) in izip!(
                    qmf_real[num_qmf_bands_lf..].iter(),
                    qmf_imag[num_qmf_bands_lf..].iter(),
                    hybrid[hyb_offset..].iter_mut()
                )
                .take(usize::from(self.num_bands) - num_qmf_bands_lf)
                {
                    hyb.re = *real;
                    hyb.im = *imag;
                }
            } else if let Some(hf_memory) = self.hf_memory.as_deref_mut() {
                // HF delay compensation.
                {
                    let (hf_real, hf_imag) = hf_memory[self.hf_pos]
                        .split_at(usize::from(self.num_bands) - num_qmf_bands_lf);

                    for (real, imag, hyb) in izip!(
                        hf_real.iter(),
                        hf_imag.iter(),
                        hybrid[hyb_offset..].iter_mut()
                    )
                    .take(usize::from(self.num_bands) - num_qmf_bands_lf)
                    {
                        hyb.re = *real;
                        hyb.im = *imag;
                    }
                }

                hf_memory[self.hf_pos][..usize::from(self.num_bands) - num_qmf_bands_lf]
                    .copy_from_slice(
                        &qmf_real[num_qmf_bands_lf
                            ..num_qmf_bands_lf + usize::from(self.num_bands) - num_qmf_bands_lf],
                    );
                hf_memory[self.hf_pos][(usize::from(self.num_bands) - num_qmf_bands_lf)
                    ..(usize::from(self.num_bands) - num_qmf_bands_lf)
                        + usize::from(self.num_bands)
                        - num_qmf_bands_lf]
                    .copy_from_slice(
                        &qmf_imag[num_qmf_bands_lf
                            ..num_qmf_bands_lf + usize::from(self.num_bands) - num_qmf_bands_lf],
                    );

                self.hf_pos += 1;
                if self.hf_pos >= usize::from(self.setup.filter_delay) {
                    self.hf_pos = 0;
                }
            } else {
                return -1;
            }
        }
        err
    }

    /// De-inits dynamic hybrid analysis filter bank memory.
    pub fn deallocate_hf_memory(&mut self) {
        self.hf_memory = None;
    }

    fn k_channel_filtering(
        qmf: &[f32],
        read_idx: &[usize; PROTO_LEN],
        hybrid: &mut [Complex<f32>],
        hybrid_config: i8,
    ) -> i32 {
        let mut err = 0;
        let invert = hybrid_config.is_negative();
        match hybrid_config {
            2 | -2 => Self::dual_channel_filtering(qmf, read_idx, &mut hybrid[..2], invert),
            -8 => Self::eight_channel_filtering(qmf, read_idx, hybrid),
            _ => err = -1,
        }
        err
    }

    fn eight_channel_filtering(
        qmf: &[f32],
        read_idx: &[usize; PROTO_LEN],
        hybrid: &mut [Complex<f32>],
    ) {
        let (qmf_real, qmf_imag) = qmf.split_at(PROTO_LEN);

        let mut mfft: [Complex<f32>; 8] = [Complex {
            re: 0.0f32,
            im: 0.0f32,
        }; 8];

        // Pre twiddeling.
        mfft[0].re = qmf_real[read_idx[6]] / 8.0_f32;
        mfft[0].im = qmf_imag[read_idx[6]] / 8.0_f32;

        let mut accu1 = Complex {
            re: qmf_real[read_idx[7]],
            im: qmf_imag[read_idx[7]],
        };
        mfft[1] = accu1 * HYB_FILTER_COEF8[1];

        accu1.re = qmf_real[read_idx[0]];
        accu1.im = qmf_imag[read_idx[0]];
        accu1 *= HYB_FILTER_COEF8[2];

        let mut accu2 = Complex {
            re: qmf_real[read_idx[8]],
            im: qmf_imag[read_idx[8]],
        };
        accu2 *= HYB_FILTER_COEF8[3];

        mfft[2] = accu1 + accu2;

        accu1.re = qmf_real[read_idx[1]];
        accu1.im = qmf_imag[read_idx[1]];
        accu1 *= HYB_FILTER_COEF8[4];

        accu2.re = qmf_real[read_idx[9]];
        accu2.im = qmf_imag[read_idx[9]];
        accu2 *= HYB_FILTER_COEF8[5];

        mfft[3] = accu1 + accu2;

        mfft[4].re = qmf_imag[read_idx[10]] * HYB_FILTER_COEF8[7].im
            - qmf_imag[read_idx[2]] * HYB_FILTER_COEF8[6].im;
        mfft[4].im = qmf_real[read_idx[2]] * HYB_FILTER_COEF8[6].im
            - qmf_real[read_idx[10]] * HYB_FILTER_COEF8[7].im;

        accu1.re = qmf_real[read_idx[3]];
        accu1.im = qmf_imag[read_idx[3]];
        accu1 *= HYB_FILTER_COEF8[8];

        accu2.re = qmf_real[read_idx[11]];
        accu2.im = qmf_imag[read_idx[11]];
        accu2 *= HYB_FILTER_COEF8[9];

        mfft[5] = accu1 + accu2;

        accu1.re = qmf_real[read_idx[4]];
        accu1.im = qmf_imag[read_idx[4]];
        accu1 *= HYB_FILTER_COEF8[10];

        accu2.re = qmf_real[read_idx[12]];
        accu2.im = qmf_imag[read_idx[12]];
        accu2 *= HYB_FILTER_COEF8[11];

        mfft[6] = accu1 + accu2;

        accu1.re = qmf_real[read_idx[5]];
        accu1.im = qmf_imag[read_idx[5]];
        accu1 *= HYB_FILTER_COEF8[12];

        mfft[7] = accu1;

        fft::fft(&mut mfft);

        hybrid[0] = mfft[7];
        hybrid[1] = mfft[0];

        hybrid[2] = mfft[6];
        hybrid[3] = mfft[1];

        hybrid[4] = mfft[2] + mfft[5];

        hybrid[5] = mfft[3] + mfft[4];
    }

    fn dual_channel_filtering(
        qmf: &[f32],
        read_idx: &[usize; PROTO_LEN],
        hybrid: &mut [Complex<f32>],
        invert: bool,
    ) {
        let invert = usize::from(invert);
        let f0 = HYB_FILTER_COEF2[0];
        let f1 = HYB_FILTER_COEF2[1];
        let f2 = HYB_FILTER_COEF2[2];

        let (qmf_real, qmf_imag) = qmf.split_at(PROTO_LEN);
        // Symmetric filter coefficients.
        let mut r1 = f0 * qmf_real[read_idx[1]] + f0 * qmf_real[read_idx[11]];
        let mut i1 = f0 * qmf_imag[read_idx[1]] + f0 * qmf_imag[read_idx[11]];
        r1 += f1 * qmf_real[read_idx[3]] + f1 * qmf_real[read_idx[9]];
        i1 += f1 * qmf_imag[read_idx[3]] + f1 * qmf_imag[read_idx[9]];
        r1 += f2 * qmf_real[read_idx[5]] + f2 * qmf_real[read_idx[7]];
        i1 += f2 * qmf_imag[read_idx[5]] + f2 * qmf_imag[read_idx[7]];

        let r6 = 0.5 * qmf_real[read_idx[6]];
        let i6 = 0.5 * qmf_imag[read_idx[6]];

        hybrid[invert].re = r6 + r1;
        hybrid[invert].im = i6 + i1;
        hybrid[1 - invert].re = r6 - r1;
        hybrid[1 - invert].im = i6 - i1;
    }
}

impl HybridSynthesis {
    /// Creates an instance of the hybrid synthesis filter bank.
    pub fn new() -> Self {
        Default::default()
    }

    /// Initializes and configures hybrid synthesis filter bank instance.
    ///
    /// # Parameters
    ///
    /// - `num_qmf_bands`:     Number of qmf bands to be processed.
    pub fn init(&mut self, num_bands: usize) {
        self.setup = &SETUP_3_10;
        self.num_bands = num_bands;
    }

    /// Applies hybrid synthesis filter bank on hybrid data.
    ///
    /// # Parameters
    ///
    /// - `hybrid`:          Hybrid data (complex).
    /// - `qmf_real`:        QMF real output data.
    /// - `qmf_imag`:        QMF imag output data.
    pub fn apply(&self, hybrid: &[Complex<f32>], qmf_real: &mut [f32], qmf_imag: &mut [f32]) {
        let mut hyb_offset = 0;

        for k in 0..usize::from(self.setup.num_qmf_bands) {
            let num_hyb_bands = usize::from(self.setup.num_hyb_bands[k]);
            let mut accu1 = 0.0f32;
            let mut accu2 = 0.0f32;

            for n in 0..num_hyb_bands {
                accu1 += hybrid[hyb_offset + n].re;
                accu2 += hybrid[hyb_offset + n].im;
            }
            qmf_real[k] = accu1;
            qmf_imag[k] = accu2;
            hyb_offset += num_hyb_bands;
        }
        if self.num_bands > usize::from(self.setup.num_qmf_bands) {
            for i in 0..self.num_bands - usize::from(self.setup.num_qmf_bands) {
                qmf_real[usize::from(self.setup.num_qmf_bands) + i] = hybrid[hyb_offset + i].re;
                qmf_imag[usize::from(self.setup.num_qmf_bands) + i] = hybrid[hyb_offset + i].im;
            }
        }
    }
}

#[test]
fn test_initializing_analysis() {
    let mut analysis = HybridAnalysis::new(true);
    analysis.init(3, true);
}

#[test]
fn test_initializing_synthesis() {
    let mut synthesis = HybridSynthesis::new();
    synthesis.init(64);
}
