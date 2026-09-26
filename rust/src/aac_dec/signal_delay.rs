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
//! Signal delay compensation
//!
//! Compensates signal delay in number of samples.
use crate::aac_dec::constants::*;

#[repr(C)]
#[derive(Debug)]
/// Structure representing one delay element for multiple channels.
pub struct SignalDelay {
    /// Delay line (delay in number of samples).
    delay_line: Vec<f32>,
}

impl SignalDelay {
    /// Creates delay element for multiple channels with fixed delay value.
    ///
    /// # Parameters
    ///
    /// - `delay`: Delay in number of samples.
    pub fn create(delay: usize) -> Self {
        SignalDelay {
            delay_line: vec![0.0; delay],
        }
    }

    /// Applies delay to one channel (non-interleaved storage assumed).
    ///
    /// # Parameters
    ///
    /// - `time_buffer`: Signal to be delayed.
    pub fn apply(&mut self, time_buffer: &mut [f32]) {
        let frame_length = time_buffer.len();
        let delay = self.delay_line.len();

        if delay > 0 {
            let mut tmp: [f32; MAX_FRAMESIZE] = [0.0; MAX_FRAMESIZE];
            if frame_length >= delay {
                tmp[..delay].copy_from_slice(&time_buffer[(frame_length - delay)..]);
                time_buffer.copy_within(..(frame_length - delay), delay);
                time_buffer[..delay].copy_from_slice(&self.delay_line);
                self.delay_line.copy_from_slice(&tmp[..delay]);
            } else {
                tmp[..frame_length].copy_from_slice(time_buffer);
                time_buffer.copy_from_slice(&self.delay_line[..frame_length]);
                self.delay_line.copy_within(frame_length..delay, 0);
                self.delay_line[(delay - frame_length)..].copy_from_slice(&tmp[..frame_length]);
            }
        }
    }

    /// Destroy delay element.
    pub fn destroy(&mut self) {
        self.delay_line.clear();
        self.delay_line.shrink_to_fit();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_delay_apply_delay_gt_frame_length() {
        let mut ref_delay = SignalDelay {
            delay_line: vec![10.0, 20.0, 30.0],
        };
        let ref_buffer: &mut [f32; 8] = &mut [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];

        ref_delay.apply(ref_buffer);

        let expected_delay_line: Vec<f32> = vec![6.0, 7.0, 8.0];

        assert_eq!(ref_delay.delay_line, expected_delay_line);
    }

    #[test]
    fn test_delay_apply_delay_lt_frame_length() {
        let mut ref_delay = SignalDelay {
            delay_line: (10..=100).step_by(10).map(|x| x as f32).collect(),
        };
        let ref_buffer: &mut [f32; 8] = &mut [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];

        ref_delay.apply(ref_buffer);

        let expected_delay_line: Vec<f32> =
            vec![90.0, 100.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];

        assert_eq!(ref_delay.delay_line, expected_delay_line);
    }
}
