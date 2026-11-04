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
//! MDCT implementation
//!
//! The module features an inverse MDCT which transforms data from the
//! frequency to the time domain. The implementation uses the `dct` module
//! of the `common` crate.

extern crate num_complex;

use super::dct::dctiv;
use itertools::izip;
use num_complex::Complex;

/// MDCT
///
/// # Inverse MDCT
///
/// For performing an inverse MDCT, an overlap buffer size has to be
/// provided at initialization time. Before doing the actual transformation,
/// window parameters can be adapted if need be.
/// The actual transformation needs the left and right slope of a
/// transformation window, the spectrum input (or spectra, along with the
/// number of spectra, in case of short blocks) and an output buffer to write
/// the time domain output samples to. Additionally, samples can be scaled
/// by a given scaling value.
/// Maximum allowed framelength is 1024.
///
/// # Examples
///
/// Do an IMDCT on a given spectrum
/// * Overlap window size is 512
///
/// ```
/// use aac::common::mdct::Mdct;
/// use num_complex::Complex;
///
/// let mut output: &mut [f32] = &mut [0.0; 1024]; // buffer for output time samples
/// let mut spectrum: &mut [f32] = &mut [0.0; 1024]; // spectrum input
/// let mut overlap_buf_size: usize = 512; // overlap buffer size
/// let n_spec: usize = 1; // number of spectra
///
/// static WLS: &[Complex<f32>; 128] = &[Complex { re: 0.0, im: 0.0 }; 128];
/// static WRS: &[Complex<f32>; 128] = &[Complex { re: 0.0, im: 0.0 }; 128];
/// let mut mdct = Mdct::new(overlap_buf_size).unwrap();
/// mdct.imdct(output, spectrum, n_spec, &mut None, WLS, WRS);
/// ```
#[derive(Default, Debug)]
#[repr(C)]
pub struct Mdct {
    pub overlap: Vec<f32>, // reference to overlap
    pub ov_offset: usize,  // overlap time data fill level

    pub prev_wrs: Option<&'static [Complex<f32>]>, // reference to previous right window slope
    pub prev_tl: usize,                            // previous transform length
    pub prev_nr: usize,                            // previous right window offset
}

impl Mdct {
    /// Create an MDCT instance
    ///
    /// For initializing it needs an input buffer that can hold the
    /// complete overlap.
    pub fn new(overlap_max_size: usize) -> Option<Mdct> {
        Some(Mdct {
            overlap: vec![0.0f32; overlap_max_size],
            ..Default::default()
        })
    }

    /// Adapt MDCT windows sizes
    ///
    /// If e.g. fr_previous does not match fl, sizes will be adapted as needed.
    pub fn adapt_parameters(
        &mut self,
        nl: &mut usize,
        wls: &'static [Complex<f32>],
        fl_ref: &mut usize,
        n_out_samples: usize,
    ) {
        let mut use_current = false;
        let mut use_previous = false;
        let fl = *fl_ref;
        let mut prev_nr_prel = self.prev_nr as i16; // preliminary prev_nr, can become negative

        debug_assert!(wls.len() * 2 == fl);

        if self.prev_tl == 0 {
            self.prev_wrs = Some(wls);
            prev_nr_prel = (n_out_samples as i16 - fl as i16) >> 1;
            self.prev_tl = n_out_samples;
            self.ov_offset = 0;
            use_current = true;
        }

        let prev_fr = self.prev_wrs.unwrap().len() * 2;
        let window_diff = (prev_fr as i16 - fl as i16) >> 1;

        // Check if the previous window slope can be adjusted to match the current window slope.
        if prev_nr_prel + window_diff > 0 {
            use_current = true;
        }

        // Check if the current window slope can be adjusted to match the previous window slope
        if *nl as i16 > window_diff {
            use_previous = true;
        }

        // If both is possible choose the larger of both window slope lengths
        if use_current && use_previous && fl < prev_fr {
            use_current = false;
        }

        // If the previous transform block is big enough, enlarge previous window
        // overlap. If not, shrink current window overlap.
        if use_current {
            prev_nr_prel += window_diff;
            self.prev_wrs = Some(wls);
        } else {
            *nl = (*nl as i16 - window_diff) as usize;
            *fl_ref = prev_fr;
        }

        self.prev_nr = prev_nr_prel as usize;
    }

    /// Do an inverse MDCT
    ///
    /// See imlt() for detailed description
    pub fn imdct(
        &mut self,
        output: &mut [f32],
        spectrum: &mut [f32],
        n_spec: usize,
        fac_zir: &Option<&[f32]>,
        wls: &'static [Complex<f32>],
        wrs: &'static [Complex<f32>],
    ) -> (usize, usize) {
        self.imlt(output, spectrum, n_spec, fac_zir, wls, wrs, 0.0)
    }

    /// Do an inverse MLT
    ///
    /// This program implements the inverse modulated lapped transform, a generalized
    /// version of the inverse MDCT transform. Computes the IMDCT, including a
    /// potential gain factor that is applied to the spectrum.
    ///
    /// If we pass the data block (A,B,C,D,E,F) to the FORWARD MDCT it will produce two
    /// outputs. The first one will be over the (A,B,C,D) part => (-D-Cr,A-Br) and the
    /// second one will be over the (C,D,E,F) part => (-F-Er,C-Dr), since there is an
    /// overlap between consecutive executions of the algorithm. This overlap is over the
    /// (C,D) segments. The two outputs will be passed sequentially to the DCT IV
    /// algorithm. At the INVERSE MDCT side we get two consecutive outputs from the IDCT
    /// IV algorithm, namely the same blocks: (-D-Cr,A-Br) and (-F-Er,C-Dr). The first
    /// of them is stored in the overlap buffer and the second is in the working one, which,
    /// one algorithm pass later will substitute the one residing in the overlap
    /// buffer. The IMDCT algorithm has to calculate the C and D segments from the two
    /// buffers. In order to do this we take the left part of the overlap
    /// buffer(-D-Cr,A-Br), namely (-D-Cr), and add it appropriately to the right part of
    /// the working buffer (-F-Er,C-Dr), namely (C-Dr). That way, we first get the C
    /// and later the D segment. We do this in the following way: From the right
    /// part of the working buffer(C-Dr), we subtract the flipped left part of the
    /// overlap buffer(-D-Cr):
    ///
    /// Result = (C-Dr) - flipped(-D-Cr) = C -Dr + Dr + C = 2C
    /// We divide by two and get the C segment. What we did is adding the right part of
    /// the first frame to the left part of the second one. While applying these
    /// operations, we multiply the respective segments with the appropriate window
    /// functions.
    ///
    /// In order to get the D segment we do the following:
    /// From the negated second part of the working buffer(C-Dr) we subtract the flipped
    /// first part of the overlap buffer (-D-Cr):
    ///
    /// Result= - (C -Dr) - flipped(-D-Cr) = -C +Dr +Dr +C = 2Dr.
    /// After dividing by two and flipping, we get the D segment. What we did is adding
    /// the right part of the first frame to the left part of the second one. While
    /// applying these operations, we multiply the respective segments with the
    /// appropriate window functions.
    ///
    /// Once we have obtained the C and D segments, the overlap buffer is emptied and the
    /// current buffer is stored there, so that the E and F segments are available in
    /// the next decoding call.
    ///
    /// # Parameters
    ///
    /// - `output`: Time domain output.
    /// - `spectrum`: MDCT spectrum input.
    /// - `n_spec`: number of MDCT spectra in a frame.
    /// - `fac_zir`: Optional FAC buffer, incl. FAC ZIR.
    /// - `wls`: Left window slope.
    /// - `wrs`: Right window slope.
    /// - `gain`: Optional scaling of signal.
    ///
    /// # Return
    ///
    /// - (`written samples`, `read_samples_fac`)
    #[expect(clippy::too_many_arguments)]
    pub fn imlt(
        &mut self,
        output: &mut [f32],
        spectrum: &mut [f32],
        n_spec: usize,
        fac_zir: &Option<&[f32]>,
        wls: &'static [Complex<f32>],
        wrs: &'static [Complex<f32>],
        gain: f32,
    ) -> (usize, usize) {
        let mut n_samples_written: usize = 0; // samples written to output buffer
        const OVL_SIZE_MAX: usize = 512; // max size is max tl/2. max tl size is a frame.

        // Derive length information
        let n_samples_to_write = output.len();
        let tl = spectrum.len() / n_spec;
        let mut fl: usize = wls.len() * 2;
        let fr: usize = wrs.len() * 2;
        let mut nl: usize = (tl - fl) >> 1;
        let nr: usize = (tl - fr) >> 1;

        // Detect FRprevious / FL mismatches and override parameters accordingly
        if self.prev_wrs.is_none() || self.prev_wrs.unwrap().len() * 2 != fl {
            self.adapt_parameters(&mut nl, wls, &mut fl, n_samples_to_write);
        }

        // Get overlap time data
        if n_samples_to_write > n_samples_written {
            output[..self.ov_offset].copy_from_slice(&self.overlap[..self.ov_offset]);
            n_samples_written = self.ov_offset;
            self.ov_offset = 0;
        }

        // Save overlap frequency data in local buffer
        let mut ov_cp_in: [f32; OVL_SIZE_MAX] = [0.0; OVL_SIZE_MAX];
        let mut ov_cp_in_len = fl / 2 + self.prev_nr;

        let mut n_elements_read_fac = 0;
        let mut is_filled_fac = (*fac_zir).is_some();

        // Read first spectrum from overlap buffer
        ov_cp_in[0..ov_cp_in_len]
            .copy_from_slice(&self.overlap[self.overlap.len() - ov_cp_in_len..]);

        for spectrum_current in spectrum.chunks_exact_mut(tl).take(n_spec) {
            // Detect FRprevious / FL mismatches and override parameters accordingly
            if self.prev_wrs.is_none() || self.prev_wrs.unwrap().len() * 2 != fl {
                self.adapt_parameters(&mut nl, wls, &mut fl, n_samples_to_write);
            }
            let fl_half = fl / 2;

            // Determine if overlap buffer is needed
            let divert_out0_to_ov = n_samples_written + self.prev_nr + fl_half > n_samples_to_write;
            let divert_out1_to_ov = n_samples_written + self.prev_nr + fl + nl > n_samples_to_write;

            let output_len = output.len();
            let overlap_len = self.overlap.len();
            // Output buffer of this windows's spectrum
            let output_current = &mut output
                [n_samples_written..(n_samples_written + self.prev_nr + fl + nl).min(output_len)];
            // Overlap buffer of this windows's spectrum
            let overlap_current = &mut self.overlap
                [self.ov_offset..(self.ov_offset + self.prev_nr + fl + nl).min(overlap_len)];

            // DCT IV of current spectrum
            dctiv(spectrum_current);

            // Optional scaling of time domain - not yet windowed - of current spectrum
            let mut len_inv = (tl as f32).recip();
            if gain != 0.0 {
                len_inv *= gain;
            }
            for iter in spectrum_current.iter_mut().take(tl) {
                *iter *= len_inv;
            }

            // Here we implement a simplified version of what happens after this
            // piece of code (see the comments below). We implement the folding of C and
            // D segments from (-D-Cr) but D is zero, because in this part of the MDCT
            // sequence the window coefficients with which D must be multiplied are zero.
            {
                // Setup window slices

                // Input of overlap buffer used in this scope
                let input_that = &mut ov_cp_in[ov_cp_in_len - self.prev_nr..ov_cp_in_len];
                // Output buffer used in this scope
                let output_this: &mut [f32];

                if !divert_out0_to_ov {
                    // Account output samples.
                    output_this = &mut output_current[..self.prev_nr];
                    n_samples_written += self.prev_nr;
                } else {
                    // Divert output first half to overlap buffer if we already
                    // got enough output samples.
                    output_this = &mut overlap_current[..self.prev_nr];
                    self.ov_offset += self.prev_nr;
                }

                // NR output samples 0 .. NR. -overlap[TL/2..TL/2-NR]
                if !is_filled_fac || (self.prev_nr != fl_half) {
                    for (out_data, in_data) in izip!(
                        output_this.iter_mut(),
                        input_that[..self.prev_nr].iter().rev()
                    )
                    .take(self.prev_nr)
                    {
                        *out_data = -*in_data;
                    }
                } else {
                    // For ACELP -> TCX20 -> FD short, add FAC ZIR on NR signal part.
                    let fac_zir_this = (*fac_zir).unwrap();
                    for (out_data, in_data, fac_data) in izip!(
                        output_this.iter_mut(),
                        input_that[..self.prev_nr].iter().rev(),
                        fac_zir_this.iter()
                    )
                    .take(self.prev_nr)
                    {
                        *out_data = -*in_data + *fac_data;
                    }
                    n_elements_read_fac += self.prev_nr;
                    //assert!(n_elements_read_fac >= (*fac_zir).unwrap().len());
                    //if n_elements_read_fac >= fac_zir_this.len() {
                    is_filled_fac = false;
                    //}
                }
            }

            // Output samples before window crossing point NR .. TL/2.
            // -overlap[TL/2-NR..TL/2-NR-FL/2] + current[NR..TL/2]
            // Output samples after window crossing point TL/2 .. TL/2+FL/2.
            // -overlap[0..FL/2] - current[TL/2..FL/2]
            {
                // Setup window slices

                // Buffer for first part of output of this scope
                let output_this: &mut [f32];
                // Buffer for second part of output of this scope
                let output_that: &mut [f32];
                // Spectrum input buffer used in this scope
                let input_this = &spectrum_current[tl - fl_half..];
                // Input of overlap buffer used in this scope
                let input_that = &mut ov_cp_in
                    [ov_cp_in_len - self.prev_nr - fl_half..ov_cp_in_len - self.prev_nr];

                if divert_out0_to_ov {
                    // Use overlap buffer for all output
                    (output_this, output_that) =
                        overlap_current[self.prev_nr..self.prev_nr + fl].split_at_mut(fl_half);
                    self.ov_offset += fl;
                } else if divert_out1_to_ov {
                    // Divert only output_that to overlap buffer
                    output_this = &mut output_current[self.prev_nr..self.prev_nr + fl_half];
                    output_that = &mut overlap_current[..fl_half];
                    n_samples_written += fl_half;
                    self.ov_offset += fl_half;
                } else {
                    // Overlap buffer not needed --> only use output buffer
                    (output_this, output_that) =
                        output_current[self.prev_nr..self.prev_nr + fl].split_at_mut(fl_half);
                    n_samples_written += fl;
                }

                let window = self.prev_wrs.unwrap();

                for i in 0..fl_half {
                    let x = Complex {
                        re: input_this[i],
                        im: -(input_that[fl_half - 1 - i]),
                    };
                    let y = x * window[i];

                    output_this[i] = y.im;
                    output_that[fl_half - 1 - i] = -y.re;
                }
            }

            if is_filled_fac {
                // Add FAC ZIR of previous ACELP -> self transition
                debug_assert!(fl_half <= 128);

                // Setup window slices
                let output_this: &mut [f32] = if divert_out0_to_ov {
                    // Work on overlap buffer
                    &mut overlap_current[self.prev_nr..self.prev_nr + fl_half]
                } else {
                    // Work on output buffer
                    &mut output_current[self.prev_nr..self.prev_nr + fl_half]
                };

                for (out_data, fac_data) in output_this
                    .iter_mut()
                    .zip((*fac_zir).unwrap().iter())
                    .take(fl_half)
                {
                    *out_data += *fac_data;
                }
                n_elements_read_fac += fl_half;
                //assert!(n_elements_read_fac >= (*fac_zir).unwrap().len());
                //if n_elements_read_fac >= (*fac_zir).unwrap().len() {
                is_filled_fac = false;
                //}
            }

            // NL output samples TL/2+FL/2..TL. - current[FL/2..0]
            // Here we implement a simplified version of what happens above this
            // piece of code (see the comments above). We implement the folding of C and D
            // segments from (C-Dr) but C is zero, because in this part of the MDCT
            // sequence the window coefficients that are multiplied by C are zero.
            {
                // Setup window slices

                // Spectrum input buffer of this scope
                let input_this = &spectrum_current[tl - fl_half - nl..tl - fl_half];
                // Output buffer for this scope
                let output_this: &mut [f32];

                if divert_out0_to_ov {
                    // Use overlap buffer
                    output_this = &mut overlap_current[self.prev_nr + fl..self.prev_nr + fl + nl];
                    self.ov_offset += nl;
                } else if divert_out1_to_ov {
                    // Use overlap buffer
                    output_this = &mut overlap_current[fl_half..fl_half + nl];
                    self.ov_offset += nl;
                } else {
                    // Overlap buffer not needed --> only use output buffer
                    output_this = &mut output_current[self.prev_nr + fl..self.prev_nr + fl + nl];
                    n_samples_written += nl;
                }

                for i in 0..nl {
                    output_this[i] = -input_this[nl - 1 - i];
                }
            }

            // Set overlap source pointer for next window
            ov_cp_in[0..tl / 2].copy_from_slice(&spectrum_current[0..tl / 2]);
            ov_cp_in_len = tl / 2;

            // Previous window values
            self.prev_nr = nr;
            self.prev_tl = tl;
            self.prev_wrs = Some(wrs);
        }

        // Save overlap
        let ov_len = self.overlap.len();
        self.overlap[ov_len - tl / 2..]
            .copy_from_slice(&spectrum[(n_spec - 1) * tl..(n_spec - 1) * tl + tl / 2]);

        (n_samples_written, n_elements_read_fac)
    }

    /// Get overlap and right window offset
    pub fn get_ov_and_nr(&mut self, output: &mut [f32]) -> usize {
        let mut n_samples = output.len();
        let nt = self.ov_offset.min(n_samples);
        n_samples -= nt;
        let nf = self.prev_nr.min(n_samples);

        output[..nt].copy_from_slice(&self.overlap[..nt]);

        for (out_data, ov_data) in
            izip!(output[nt..].iter_mut(), self.overlap.iter().rev()).take(nf)
        {
            *out_data = -*ov_data;
        }

        nt + nf
    }

    /// Drain IMDCT, i.e. get remaining bits out of overlap buffer.
    pub fn drain(&mut self, output: &mut [f32]) -> usize {
        let mut buffered_samples: usize = 0;

        if !output.is_empty() {
            buffered_samples = self.ov_offset;

            assert!(buffered_samples <= output.len());

            if buffered_samples > 0 {
                output[..buffered_samples].copy_from_slice(&self.overlap[..buffered_samples]);
                self.ov_offset = 0;
            }
        }
        buffered_samples
    }

    /// Clear overlap buffer, i.e. all values are set to zero.
    pub fn clear_overlap(&mut self) {
        let len = self.overlap.len();
        self.overlap.resize(0, 0.0);
        self.overlap.resize(len, 0.0);
    }

    /// Get transform length of previous window.
    pub fn prev_tl(&self) -> usize {
        self.prev_tl
    }

    /// Get previous window, incl. transform length and right window offset.
    pub fn get_prev_window(
        &self,
        nr: Option<&mut usize>,
        tl: Option<&mut usize>,
    ) -> Option<&[Complex<f32>]> {
        if let Some(nr_ref) = nr {
            *nr_ref = self.prev_nr;
        };
        if let Some(tl_ref) = tl {
            *tl_ref = self.prev_tl;
        };
        self.prev_wrs
    }

    /// Set previous window, incl. transform length and right window offset.
    pub fn set_prev_window(&mut self, wrs: Option<&'static [Complex<f32>]>, nr: usize, tl: usize) {
        self.prev_wrs = wrs;
        self.prev_nr = nr;
        self.prev_tl = tl;
    }

    /// Get overlap buffer.
    pub fn overlap_buf(&mut self) -> &mut Vec<f32> {
        &mut self.overlap
    }

    /// Get overlap buffer offset, i.e. the buffer's time data fill level.
    pub fn ov_offset(&self) -> usize {
        self.ov_offset
    }

    /// Set overlap buffer offset, i.e. the buffer's time data fill level.
    pub fn set_ov_offset(&mut self, ov_offset: usize) {
        self.ov_offset = ov_offset
    }

    /// Perform forward MDCT on a block
    ///
    /// This function implements the forward MDCT transform on an input block of data.
    /// The input block is in a form (A,B,C,D) where A,B,C and D are the respective
    /// 1/4th segments of the block. The program takes the input block and folds it in
    /// the form: (-D-Cr,A-Br).
    /// This block is twice shorter and here the 'r' suffix denotes flipping of the
    /// sequence (reversing the order of the samples). While folding the
    /// input block in the above mentioned shorter block the program windows the data.
    /// Because the two operations (windowing and folding) are not implemented
    /// sequentially, but together the program's structure is not easy to understand.
    /// Once the output (already windowed) block (-D-Cr,A-Br) is ready it is passed to
    /// the DCT IV for processing.
    pub fn mdct(
        &mut self,
        time_data: &mut [f32],
        mdct_data: &mut [f32],
        n_spec: usize,
        right_window_part: &'static [Complex<f32>],
    ) -> i32 {
        const MDCT_FAC_FL: f32 = 2.0;

        // Derive length information
        let no_in_samples = time_data.len() / 2;
        let tl = mdct_data.len() / n_spec;
        let fr: usize = right_window_part.len() * 2;
        let mut prev_fr = match self.prev_wrs {
            Some(slice) => slice.len() * 2,
            None => 0,
        };

        let wrs = right_window_part;

        // Detect FRprevious / FL mismatches and override parameters accordingly
        if prev_fr == 0 {
            // At start just initialize and pass parameters as they are
            prev_fr = fr;
            self.prev_wrs = Some(wrs);
            self.prev_tl = tl;
        }

        // Derive NR
        let nr: usize = (tl - fr) >> 1;

        // Skip input samples if tl is smaller than block size
        let input = &mut time_data[((no_in_samples - tl) >> 1)..];

        // windowing
        for (input_current, output_current) in izip!(
            input.windows(2 * tl).step_by(tl).take(n_spec),
            mdct_data.chunks_exact_mut(tl).take(n_spec)
        ) {
            let wls = self.prev_wrs.unwrap();
            let fl = prev_fr;
            let nl = (tl - fl) >> 1;

            // Here we implement a simplified version of what happens after this
            // piece of code (see the comments below). We implement the folding of A and B
            // segments to (A-Br) but A is zero, because in this part of the MDCT sequence
            // the window coefficients with which A must be multiplied are zero.
            let input_neg_rev_b = &input_current[(tl - nl)..tl];
            let mdct_neg_rev_b = &mut output_current[(tl / 2)..((tl / 2) + nl)];
            for (output_nr_b, &input_nr_b) in
                izip!(mdct_neg_rev_b.iter_mut(), input_neg_rev_b.iter().rev())
            {
                *output_nr_b = -input_nr_b * MDCT_FAC_FL;
            }

            // Implements the folding and windowing of the left part of the sequence,
            // that is segments A and B. The A segment is multiplied by the respective left
            // window coefficient and placed in a temporary variable.
            // After this the B segment taken in reverse order is multiplied by the left
            // window and subtracted from the previously derived temporary variable, so
            // that finally we implement the A-Br operation. This output is written to the
            // right part of the MDCT output : (-D-Cr,A-Br).
            // The (A-Br) data is written to the output buffer (mdct_data) without being
            // flipped.
            let mdct_fold_win_a_b =
                &mut output_current[((tl / 2) + nl)..((tl / 2) + nl + (fl / 2))];
            let input_fold_win_a_b_forward = &input_current[nl..(nl + (fl / 2))];
            let input_fold_win_a_b_backward = &input_current[(tl - nl - (fl / 2))..(tl - nl)];
            let wls_fold_win_a_b: &[Complex<f32>] = &wls[0..(fl / 2)];

            for (output_fw_ab, &input_fw_ab_f, &input_fw_ab_b, &wls_fw_ab) in izip!(
                mdct_fold_win_a_b.iter_mut(),
                input_fold_win_a_b_forward.iter(),
                input_fold_win_a_b_backward.iter().rev(),
                wls_fold_win_a_b.iter()
            ) {
                *output_fw_ab =
                    (input_fw_ab_f * wls_fw_ab.im - input_fw_ab_b * wls_fw_ab.re) * MDCT_FAC_FL;
            }

            // Right window slope offset
            // Here we implement a simplified version of what happens after the this
            // piece of code (see the comments below). We implement the folding of C and D
            // segments to (-D-Cr) but D is zero, because in this part of the MDCT sequence
            // the window coefficients with which D must be multiplied are zero.
            let input_neg_rev_c = &input_current[tl..(tl + nr)];
            let output_neg_rev_c = &mut output_current[((tl / 2) - nr)..(tl / 2)];

            for (output_nr_c, &input_nr_c) in
                izip!(output_neg_rev_c.iter_mut(), input_neg_rev_c.iter().rev())
            {
                *output_nr_c = -input_nr_c * MDCT_FAC_FL;
            }

            // Implements the folding and windowing of the right part of the sequence,
            // that is, segments C and D. The C segment is multiplied by the respective
            // right window coefficient and placed in a temporary variable.
            // After this the D segment taken in reverse order is multiplied by the right
            // window and added from the previously derived temporary variable, so that we
            // get (C+Dr) operation. This output is negated to get (-C-Dr) and written to
            // the left part of the MDCT output while being reversed (flipped) at the same
            // time, so that from (-C-Dr) we get (-D-Cr)=> (-D-Cr,A-Br).
            let mdct_fold_win_c_d =
                &mut output_current[((tl / 2) - nr - (fr / 2))..((tl / 2) - nr)];
            let input_fold_win_c_d_forward = &input_current[(tl + nr)..(tl + nr + (fr / 2))];
            let input_fold_win_c_d_backward =
                &input_current[((tl * 2) - nr - (fr / 2))..((tl * 2) - nr)];
            let wrs_fold_win_c_d: &[Complex<f32>] = &wrs[0..(fr / 2)];

            for (output_fw_cd, &input_fw_cd_f, &input_fw_cd_b, &wrs_fw_cd) in izip!(
                mdct_fold_win_c_d.iter_mut().rev(),
                input_fold_win_c_d_forward.iter(),
                input_fold_win_c_d_backward.iter().rev(),
                wrs_fold_win_c_d.iter()
            ) {
                *output_fw_cd =
                    -(input_fw_cd_b * wrs_fw_cd.im + input_fw_cd_f * wrs_fw_cd.re) * MDCT_FAC_FL;
            }

            // We pass the shortened folded data (-D-Cr,A-Br) to the MDCT function
            dctiv(&mut output_current[..]);

            self.prev_wrs = Some(wrs);
            self.prev_tl = tl;
        }

        (n_spec * tl) as i32
    }
}
