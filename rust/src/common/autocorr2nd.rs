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
//! Autocorrelation implementation

use itertools::izip;
use num_complex::Complex;

/// Autocorrelation coefficients
#[repr(C)]
#[derive(Debug, Default)]
pub struct AutocorrCoeffs {
    r00r: f32,
    r11r: f32,
    r22r: f32,
    r01: Complex<f32>,
    r12: Complex<f32>,
    r02: Complex<f32>,
    det: f32,
}

pub const LPC_ORDER: usize = 2;

impl AutocorrCoeffs {
    /// Calculates second order autocorrelation using 2 accumulators.
    ///
    /// # Parameters
    ///
    /// - `buffer`: Input buffer with complex samples
    pub fn calculate(&mut self, buffer: &[Complex<f32>]) {
        let len = buffer.len() - LPC_ORDER;

        let mut accu1 = 0f32;
        let mut accu3 = Complex::new(0f32, 0f32);
        let mut accu5 = buffer[2] * buffer[0].conj();
        for (i_0, i_1, i_2) in izip!(
            // Iterating over triplets of consecutive buffer elements
            &buffer[1..len],
            &buffer[2..len + 1],
            &buffer[3..len + 2],
        ) {
            accu1 += i_0.norm_sqr();
            accu3 += i_0.conj() * i_1;
            accu5 += i_2 * i_0.conj();
        }

        let accu2 = buffer[0].norm_sqr() + accu1;
        accu1 += buffer[len].norm_sqr();
        let accu0 = buffer[len + 1].norm_sqr() - buffer[1].norm_sqr() + accu1;
        let accu4 = buffer[1] * buffer[0].conj() + accu3;
        accu3 += buffer[len + 1] * buffer[len].conj();

        self.r00r = accu0;
        self.r11r = accu1;
        self.r22r = accu2;
        self.r01 = accu3;
        self.r12 = accu4;
        self.r02 = accu5;

        self.det = self.r11r * self.r22r - self.r12.norm_sqr();
    }

    /// Convenience function that creates a new [`AutocorrCoeffs`], calls [`calculate`]
    /// with the given buffer on it and returns the result.
    ///
    /// [`calculate`]: AutocorrCoeffs::calculate
    ///
    /// # Parameters
    ///
    /// - `buffer`: Input buffer with complex samples
    pub fn from_buffer(buffer: &[Complex<f32>]) -> Self {
        let mut ac = Self::default();
        ac.calculate(buffer);
        ac
    }

    pub fn r00r(&self) -> f32 {
        self.r00r
    }

    pub fn r11r(&self) -> f32 {
        self.r11r
    }

    pub fn r22r(&self) -> f32 {
        self.r22r
    }

    pub fn r01(&self) -> Complex<f32> {
        self.r01
    }

    pub fn r12(&self) -> Complex<f32> {
        self.r12
    }

    pub fn r02(&self) -> Complex<f32> {
        self.r02
    }

    pub fn det(&self) -> f32 {
        self.det
    }
}

#[cfg(test)]
mod test {
    use super::*;

    // Only basic sanity checks here.

    #[test]
    fn constant_zeros() {
        const BUFFER_LEN: usize = 20;

        let buf = [Complex::new(0f32, 0f32); BUFFER_LEN];
        let ac = AutocorrCoeffs::from_buffer(&buf);

        let ref_real = 0f32;
        let ref_cplx = Complex::new(ref_real, 0f32);

        assert_eq!(ac.r01, ref_cplx);
        assert_eq!(ac.r12, ref_cplx);
        assert_eq!(ac.r02, ref_cplx);
        assert_eq!(ac.r00r, ref_real);
        assert_eq!(ac.r11r, ref_real);
        assert_eq!(ac.r22r, ref_real);
        assert_eq!(ac.det, 0f32);
    }

    #[test]
    fn constant_ones() {
        const BUFFER_LEN: usize = 10;

        let buf = [Complex::new(1f32, 0f32); BUFFER_LEN];
        let ac = AutocorrCoeffs::from_buffer(&buf);

        let ref_real = (BUFFER_LEN - LPC_ORDER) as f32;
        let ref_cplx = Complex::new(ref_real, 0f32);

        assert_eq!(ac.r01, ref_cplx);
        assert_eq!(ac.r12, ref_cplx);
        assert_eq!(ac.r02, ref_cplx);
        assert_eq!(ac.r00r, ref_real);
        assert_eq!(ac.r11r, ref_real);
        assert_eq!(ac.r22r, ref_real);
        assert_eq!(ac.det, 0f32);
    }

    #[test]
    fn alternating_cplx() {
        const BUFFER_LEN: usize = 4;

        let alternate = |i| {
            if i % 2 == 0 {
                Complex::from_polar(1f32, std::f32::consts::FRAC_PI_4)
            } else {
                Complex::from_polar(1f32, 3f32 * std::f32::consts::FRAC_PI_4)
            }
        };

        let buf: Vec<_> = (0..BUFFER_LEN).map(alternate).collect();
        let ac = AutocorrCoeffs::from_buffer(buf.as_slice());

        let ref_real = (BUFFER_LEN - LPC_ORDER) as f32;
        let ref_cplx = Complex::new(ref_real, 0f32);
        let zero_cplx = Complex::new(0f32, 0f32);

        let margin = 1e-6f32;
        let approx = |a: f32, b: f32| (a - b).abs() <= margin;
        let approx_cplx =
            |a: Complex<f32>, b: Complex<f32>| approx(a.re, b.re) && approx(a.im, b.im);

        assert!(approx_cplx(ac.r01, zero_cplx));
        assert!(approx_cplx(ac.r12, zero_cplx));
        assert!(approx_cplx(ac.r02, ref_cplx));
        assert!(approx(ac.r00r, ref_real));
        assert!(approx(ac.r11r, ref_real));
        assert!(approx(ac.r22r, ref_real));
        assert!(approx(ac.det, ref_real * ref_real));
    }
}
