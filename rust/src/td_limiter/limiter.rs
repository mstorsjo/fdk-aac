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
//! Time domain PCM limiter
//!
//! Hard PCM limiter for clipping prevention.

use itertools::izip;

/// Limiter error codes.
#[repr(C)]
#[derive(Debug, PartialEq, Eq)]
pub enum TdLimiterError {
    Ok = 0,
    Unknown = -1,
    InvalidHandle = -99,
    InvalidParameter = -98,
}

// Defaults
/// Default threshold value.
pub const TDLIMIT_THRESHOLD_DEFAULT: f32 = 1.0;
/// Default attack  time in milliseconds.
pub const TDLIMIT_ATTACK_DEFAULT_MS: u32 = 15;
/// Default release time in milliseconds.
pub const TDLIMIT_RELEASE_DEFAULT_MS: u32 = 50;

// Min/Max
/// Minimum allowed attack time in milliseconds.
pub const TDLIMIT_MIN_ATTACK_MS: u32 = 1;
/// Maximum allowed attack time in milliseconds.
pub const TDLIMIT_MAX_ATTACK_MS: u32 = 15;

/// Minimum allowed release time in milliseconds.
pub const TDLIMIT_MIN_RELEASE_MS: u32 = 1;

/// Factor on release time to let the gain fall back to 1.0 in float precision.
const TDLIMIT_FALLBACK_FACTOR: u32 = 8;

/// Maximum sample rate usable for the time domain limiter.
pub const TDLIMIT_MAX_SAMPLERATE: u32 = 96000;

/// Default threshold value for peak limiter (-1 dBFS).
pub(crate) const TD_PEAK_LIMIT_THRESHOLD_DEFAULT: f32 = 0.89125094;
/// Default attack time for the peak limiter, expressed in miliseconds.
pub(crate) const TD_PEAK_LIMIT_ATTACK_DEFAULT_MS: u32 = 5;

/// Time domain PCM limiter structure.
#[derive(Default, Debug)]
#[repr(C)]
pub struct PcmLimiter {
    attack: u32,
    attack_const: f32,
    release_const: f32,
    attack_ms: u32,
    release_ms: u32,
    max_attack_ms: u32,
    threshold: f32,
    channels: usize,
    max_channels: usize,
    sample_rate: u32,
    max_sample_rate: u32,
    cor: f32,
    max: f32,

    max_buf: Vec<f32>,
    delay_buf: Vec<f32>,
    max_buf_idx: usize,
    delay_buf_idx: usize,
    smooth_state_0: f32,
    min_gain: f32,
    clean_samples: u32,
    release: u32,
    previous_mode: u8,
}

impl PcmLimiter {
    /// Applies limiter on given input samples.
    ///
    /// # Parameters:
    ///
    /// - `samples_in_out`: Input/Output buffer containing interleaved samples
    /// - `work_buf`: Work buffer with length of (number of samples / channel)
    ///
    /// # Return
    ///
    /// - `Result<(), TdLimiterError>`
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::td_limiter::limiter::PcmLimiter;
    /// const CHANNELS: usize = 1;
    /// const SAMPLERATE: u32 = 8000;
    /// const NUM_SAMPLES: usize = 64;
    /// let attack_ms = 1;
    /// let release_ms = 1;
    /// let threshold = 0.5;
    ///
    /// let mut samples_in_out = [1.0_f32; CHANNELS * NUM_SAMPLES];
    /// let mut work_buf = [0.0_f32; NUM_SAMPLES];
    ///
    /// let mut limiter = PcmLimiter::new(attack_ms, CHANNELS, SAMPLERATE).unwrap();
    /// let _ = limiter.set_release_ms(release_ms);
    /// let _ = limiter.set_threshold(threshold);
    /// let _ = limiter.apply(&mut samples_in_out, &mut work_buf);
    ///
    /// samples_in_out[..]
    ///     .iter()
    ///     .for_each(|g| assert!(*g <= threshold));
    /// ```
    pub fn apply(
        &mut self,
        samples_in_out: &mut [f32],
        work_buf: &mut [f32],
    ) -> Result<(), TdLimiterError> {
        let channels = self.channels;
        let attack = self.attack as usize;
        let mut delay_buf_idx = self.delay_buf_idx;
        let threshold = self.threshold;
        let num_samples = samples_in_out.len() / channels;

        debug_assert!(num_samples <= work_buf.len());

        // Preparatory step. Find local maximums and store them in an array.
        // Find the global maximum.
        let mut global_max = 0.0f32;

        if channels == 2 {
            for (sample, local_max_buff) in izip!(
                samples_in_out[..channels * num_samples].chunks_exact(channels),
                work_buf.iter_mut()
            )
            .take(num_samples)
            {
                *local_max_buff = f32::max(sample[0].abs(), sample[1].abs());
                global_max = global_max.max(*local_max_buff);
            }
        } else if channels == 6 {
            for (sample, local_max_buff) in izip!(
                samples_in_out[..channels * num_samples].chunks_exact(channels),
                work_buf.iter_mut()
            )
            .take(num_samples)
            {
                let mut local_max_a = f32::max(sample[0].abs(), sample[1].abs());
                let local_max_b = f32::max(sample[2].abs(), sample[3].abs());
                let local_max_c = f32::max(sample[4].abs(), sample[5].abs());
                local_max_a = local_max_a.max(local_max_b);
                *local_max_buff = f32::max(local_max_a, local_max_c);
                global_max = global_max.max(*local_max_buff);
            }
        } else if channels == 8 {
            for (sample, local_max_buff) in izip!(
                samples_in_out[..channels * num_samples].chunks_exact(channels),
                work_buf.iter_mut()
            )
            .take(num_samples)
            {
                let mut local_max_a = f32::max(sample[0].abs(), sample[1].abs());
                let local_max_b = f32::max(sample[2].abs(), sample[3].abs());
                let mut local_max_c = f32::max(sample[4].abs(), sample[5].abs());
                let local_max_d = f32::max(sample[6].abs(), sample[7].abs());
                local_max_a = local_max_a.max(local_max_b);
                local_max_c = local_max_c.max(local_max_d);
                *local_max_buff = f32::max(local_max_a, local_max_c);
                global_max = global_max.max(*local_max_buff);
            }
        } else {
            for (sample, local_max_buff) in izip!(
                samples_in_out[..channels * num_samples].chunks_exact(channels),
                work_buf.iter_mut()
            )
            .take(num_samples)
            {
                let mut local_max = 0.0f32;
                for data in sample.iter() {
                    local_max = local_max.max(f32::abs(*data));
                }
                *local_max_buff = local_max;
                global_max = global_max.max(local_max);
            }
        }

        // Check and control which mode (default/simplified) must be started.
        if global_max > threshold {
            self.clean_samples = 0;
        } else {
            self.clean_samples += num_samples as u32;
        }

        // Simplified mode. The limiter is not active. Just handle circular buffer input/output.
        if self.clean_samples > TDLIMIT_FALLBACK_FACTOR * self.release {
            // Copy delayed signal from delay line to output.
            let mut offset = 0;
            let mut num_samples_left = num_samples;
            let mut delay_copy_length = attack - delay_buf_idx;
            while num_samples_left > 0 {
                delay_copy_length = delay_copy_length.min(num_samples_left);

                // Swap data b/w in/out buffer and delay buffer.
                for (in_data, delay_data) in izip!(
                    samples_in_out[offset..].iter_mut(),
                    self.delay_buf[delay_buf_idx * channels..].iter_mut()
                )
                .take(delay_copy_length * channels)
                {
                    std::mem::swap(in_data, delay_data);
                }

                delay_buf_idx += delay_copy_length;
                if delay_buf_idx >= attack {
                    delay_buf_idx = 0;
                }
                offset += delay_copy_length * channels;
                num_samples_left -= delay_copy_length;
                delay_copy_length = attack - delay_buf_idx;
            }

            // If we come from the default mode, then reset state.
            if self.previous_mode == 1 {
                self.max_buf_idx = 0;
                self.max = 0.0f32;
                self.cor = 1.0f32;
                self.smooth_state_0 = 1.0f32;
                self.min_gain = 1.0f32;
                self.previous_mode = 0; // Set to simplified mode.
                let _ = &self.max_buf[..(self.attack + 1) as usize].fill(0.0f32);
            }

            // Store the circular buffer index.
            self.delay_buf_idx = delay_buf_idx;

            // Limit clean_samples growth.
            self.clean_samples = (TDLIMIT_FALLBACK_FACTOR + 1) * self.release;
        } else {
            let mut max_buf_idx = self.max_buf_idx;
            let mut gain: f32;
            let mut min_gain = 1.0f32;
            let attack_const = self.attack_const;
            let release_const = self.release_const;
            let mut max = self.max;
            let mut cor = self.cor;
            let mut smooth_state_0 = self.smooth_state_0;

            for (wb, samp_in_out_chunk) in
                izip!(work_buf.iter(), samples_in_out.chunks_exact_mut(channels)).take(num_samples)
            {
                // Set threshold as lower border to save calculations in running maximum algorithm.
                let tmp = (*wb).max(threshold);

                // Running maximum.
                let old = self.max_buf[max_buf_idx];
                self.max_buf[max_buf_idx] = tmp;

                if tmp >= max {
                    // New sample is greater than old maximum, so it is the new maximum.
                    max = tmp;
                } else if old < max {
                    // Maximum does not change, as the sample which has left the window,
                    // was not the maximum.
                } else {
                    // The old maximum has left the window. We have to search the complete
                    // buffer for the new max.
                    max = self.max_buf[0];
                    for m in self.max_buf[1..=attack].iter() {
                        max = max.max(*m);
                    }
                }
                max_buf_idx += 1;
                if max_buf_idx >= (attack + 1) {
                    max_buf_idx = 0;
                }

                // Calc gain.
                if max > threshold {
                    gain = threshold / max;
                } else {
                    gain = 1.0f32;
                }
                // Gain smoothing method: TDL_EXPONENTIAL
                // First order IIR filter with attack correction to avoid overshoots.
                // Correct the 'aiming' value of the exponential attack to avoid the
                // remaining overshoot.
                if gain < smooth_state_0 {
                    cor = cor.min((gain - 0.1f32 * smooth_state_0) * 1.11111111f32);
                } else {
                    cor = gain;
                }

                // Smoothing filter.
                if cor < smooth_state_0 {
                    // Attack
                    smooth_state_0 = attack_const * (smooth_state_0 - cor) + cor;
                    smooth_state_0 = smooth_state_0.max(gain);
                } else {
                    // Release
                    smooth_state_0 = release_const * (smooth_state_0 - cor) + cor;
                }

                gain = smooth_state_0;

                // Lookahead delay, apply gain.
                for (delay_data, samp_in_out) in izip!(
                    self.delay_buf[delay_buf_idx * channels..].iter_mut(),
                    samp_in_out_chunk.iter_mut()
                )
                .take(channels)
                {
                    // Apply gain from smoothing filter.
                    // Catch tiny overshoots that still can happen due to limited precision.
                    let mut delay_sample = *delay_data * gain;
                    delay_sample = delay_sample.min(threshold);
                    delay_sample = delay_sample.max(-threshold);
                    *delay_data = *samp_in_out;
                    *samp_in_out = delay_sample;
                }

                delay_buf_idx += 1;
                if delay_buf_idx >= attack {
                    delay_buf_idx = 0;
                }

                // Save minimum gain factor.
                min_gain = min_gain.min(gain);
            }

            self.max = max;
            self.max_buf_idx = max_buf_idx;
            self.cor = cor;
            self.delay_buf_idx = delay_buf_idx;
            self.smooth_state_0 = smooth_state_0;
            self.min_gain = min_gain;
            self.previous_mode = 1; // Set to default mode.
        }
        Ok(())
    }

    pub fn get_num_channels(&self) -> usize {
        self.channels
    }

    /// Gets delay value of limiter (in samples).
    ///
    /// Returns delay value as u32.
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::td_limiter::limiter::*;
    ///
    /// let max_attack_ms = TDLIMIT_MAX_ATTACK_MS;
    /// let max_channels = 8;
    /// let max_samplerate = 96000;
    ///
    /// let mut limiter = PcmLimiter::new(max_attack_ms, max_channels, max_samplerate).unwrap();
    /// let delay = limiter.get_delay();
    ///
    /// assert!(delay == max_attack_ms * max_samplerate / 1000);
    /// ```
    pub fn get_delay(&self) -> u32 {
        self.attack
    }

    /// Initializes limiter instance with given parameter values.
    fn init(
        &mut self,
        attack_ms: u32,
        release_ms: u32,
        sample_rate: u32,
    ) -> Result<(), TdLimiterError> {
        self.set_sample_rate(sample_rate)?;
        self.set_attack_ms(attack_ms)?;
        self.set_release_ms(release_ms)?;

        self.clean_samples = (TDLIMIT_FALLBACK_FACTOR + 1) * self.release;

        Ok(())
    }

    /// Creates a new `PcmLimiter` instance.
    ///
    /// # Parameters
    ///
    /// - `max_attack_in_ms`: Maximum and initial attack/lookahead time in milliseconds
    /// - `max_num_channels`: Maximum and initial number of channels
    /// - `max_sampling_rate`: Maximum and initial sampling rate in Hz
    ///
    /// # Return
    ///
    /// - `Result<PcmLimiter, TdLimiterError>`
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::td_limiter::limiter::*;
    ///
    /// let max_attack_ms = TDLIMIT_MAX_ATTACK_MS;
    /// let max_channels = 8;
    /// let max_samplerate = 96000;
    ///
    /// let limiter = PcmLimiter::new(max_attack_ms, max_channels, max_samplerate);
    /// ```
    pub fn new(
        max_attack_in_ms: u32,
        max_num_channels: usize,
        max_sampling_rate: u32,
    ) -> Result<Self, TdLimiterError> {
        let mut limiter = PcmLimiter {
            attack_ms: max_attack_in_ms,
            max_attack_ms: max_attack_in_ms,
            max_channels: max_num_channels,
            max_sample_rate: max_sampling_rate,
            release_ms: TDLIMIT_RELEASE_DEFAULT_MS,
            threshold: TDLIMIT_THRESHOLD_DEFAULT,
            ..Default::default()
        };

        limiter.init(
            max_attack_in_ms,
            TDLIMIT_RELEASE_DEFAULT_MS,
            max_sampling_rate,
        )?;

        limiter.set_n_channels(max_num_channels)?;

        limiter.max_buf = vec![0.0f32; (limiter.attack + 1).try_into().unwrap()];
        limiter.delay_buf = vec![0.0f32; limiter.attack as usize * max_num_channels];

        Ok(limiter)
    }

    /// Resets internal state of Limiter instance.
    fn reset(&mut self) {
        self.max_buf_idx = 0;
        self.delay_buf_idx = 0;
        self.max = 0.0;
        self.cor = 1.0;
        self.smooth_state_0 = 1.0;
        self.min_gain = 1.0;
        self.previous_mode = 0; // Assume simplified mode.

        self.max_buf.fill(0.0);
        self.delay_buf.fill(0.0);
    }

    /// Sets attack time of limiter (in milliseconds).
    ///
    /// # Parameters
    ///
    /// - `attack_ms`: Attack time in ms (Range: `[TDLIMIT_MIN_ATTACK_MS, TDLIMIT_MAX_ATTACK_MS]`).
    ///
    /// # Return
    ///
    /// - `Result<(), TdLimiterError>`
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::td_limiter::limiter::*;
    ///
    /// let max_attack_ms = TDLIMIT_MAX_ATTACK_MS;
    /// let max_channels = 8;
    /// let max_samplerate = 96000;
    ///
    /// let mut limiter = PcmLimiter::new(max_attack_ms, max_channels, max_samplerate).unwrap();
    ///
    /// assert!(limiter.set_attack_ms(0) == Err(TdLimiterError::InvalidParameter));
    /// assert!(limiter.set_attack_ms(1) == Ok(()));
    /// assert!(limiter.set_attack_ms(max_attack_ms) == Ok(()));
    /// assert!(limiter.set_attack_ms(max_attack_ms + 1) == Err(TdLimiterError::InvalidParameter));
    /// ```
    pub fn set_attack_ms(&mut self, attack_ms: u32) -> Result<(), TdLimiterError> {
        if !(1..=self.max_attack_ms).contains(&attack_ms) {
            return Err(TdLimiterError::InvalidParameter);
        }

        // Calculate attack time in samples.
        self.set_attack_samples(attack_ms, self.sample_rate)?;
        self.attack_ms = attack_ms;

        Ok(())
    }

    /// Calculates attack time in samples.
    fn set_attack_samples(
        &mut self,
        attack_ms: u32,
        sample_rate: u32,
    ) -> Result<(), TdLimiterError> {
        let attack = attack_ms * sample_rate / 1000;
        if attack == 0 {
            return Err(TdLimiterError::InvalidParameter);
        }

        self.attack_const = f32::powf(0.1f32, 1.0f32 / (attack + 1) as f32);

        // Reset in case that the attack time changes. Seamless switching is not supported, yet.
        if self.attack != attack {
            self.reset();
            self.attack = attack;
        }

        Ok(())
    }

    /// Sets number of channels.
    ///
    /// # Parameters
    ///
    /// - `num_channels`: Number of channels (<= `max_channels` specified on create)
    ///
    /// # Return
    ///
    /// - `Result<(), TdLimiterError>`
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::td_limiter::limiter::*;
    ///
    /// let max_attack_ms = TDLIMIT_MAX_ATTACK_MS;
    /// let max_channels = 8;
    /// let max_samplerate = 96000;
    ///
    /// let mut limiter = PcmLimiter::new(max_attack_ms, max_channels, max_samplerate).unwrap();
    ///
    /// assert!(limiter.set_n_channels(6) == Ok(()));
    /// assert!(limiter.set_n_channels(8) == Ok(()));
    /// assert!(limiter.set_n_channels(9) == Err(TdLimiterError::InvalidParameter));
    /// ```
    pub fn set_n_channels(&mut self, num_channels: usize) -> Result<(), TdLimiterError> {
        if num_channels > self.max_channels || num_channels == 0 {
            return Err(TdLimiterError::InvalidParameter);
        }

        if self.channels != num_channels {
            self.reset();
            self.channels = num_channels;
        }

        Ok(())
    }

    ///  Sets release time of limiter (in milliseconds).
    ///
    /// # Parameters
    ///
    /// - `release_ms`: Release time in ms. Min val: `TDLIMIT_MIN_RELEASE_MS`.
    ///
    /// # Return
    ///
    /// - `Result<(), TdLimiterError>`
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::td_limiter::limiter::*;
    ///
    /// let max_attack_ms = TDLIMIT_MAX_ATTACK_MS;
    /// let max_channels = 8;
    /// let max_samplerate = 96000;
    /// let release_ms = TDLIMIT_RELEASE_DEFAULT_MS;
    ///
    /// let mut limiter = PcmLimiter::new(max_attack_ms, max_channels, max_samplerate).unwrap();
    ///
    /// assert!(limiter.set_release_ms(30) == Ok(()));
    /// assert!(limiter.set_release_ms(release_ms) == Ok(()));
    /// assert!(limiter.set_release_ms(release_ms + 1) == Ok(()));
    /// ```
    pub fn set_release_ms(&mut self, release_ms: u32) -> Result<(), TdLimiterError> {
        if release_ms < TDLIMIT_MIN_RELEASE_MS {
            return Err(TdLimiterError::InvalidParameter);
        }
        self.release_ms = release_ms;
        self.set_release_samples(release_ms, self.sample_rate)?;

        Ok(())
    }

    fn set_release_samples(
        &mut self,
        release_ms: u32,
        sample_rate: u32,
    ) -> Result<(), TdLimiterError> {
        if release_ms == 0 {
            return Err(TdLimiterError::InvalidParameter);
        }
        self.release = release_ms * sample_rate / 1000;
        self.release_const = f32::powf(0.1f32, 1.0f32 / (self.release + 1) as f32);
        Ok(())
    }

    /// Sets sampling rate of limiter (in Hz).
    ///
    /// # Parameters
    ///
    /// - `sampling_rate`: Sampling rate in Hz (<= `max_sample_rate` specified on create)
    ///
    /// # Return
    ///
    /// - `Result<(), TdLimiterError>`
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::td_limiter::limiter::*;
    ///
    /// let max_attack_ms = TDLIMIT_MAX_ATTACK_MS;
    /// let max_channels = 8;
    /// let max_fs = 96000; // max samplerate
    ///
    /// let mut limiter = PcmLimiter::new(max_attack_ms, max_channels, max_fs).unwrap();
    ///
    /// assert!(limiter.set_sample_rate(0) == Err(TdLimiterError::InvalidParameter));
    /// assert!(limiter.set_sample_rate(max_fs) == Ok(()));
    /// assert!(limiter.set_sample_rate(max_fs + 1) == Err(TdLimiterError::InvalidParameter));
    /// ```
    pub fn set_sample_rate(&mut self, sampling_rate: u32) -> Result<(), TdLimiterError> {
        if !(1..=self.max_sample_rate).contains(&sampling_rate) {
            return Err(TdLimiterError::InvalidParameter);
        }

        if self.sample_rate != sampling_rate {
            self.reset();
            self.sample_rate = sampling_rate;

            self.set_attack_samples(self.attack_ms, self.sample_rate)?;
            self.set_release_samples(self.release_ms, self.sample_rate)?;
        }
        Ok(())
    }

    ///  Sets limiter threshold value.
    ///
    /// # Parameters
    ///
    /// - `threshold`: Limiting threshold
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::td_limiter::limiter::*;
    ///
    /// let max_attack_ms = TDLIMIT_MAX_ATTACK_MS;
    /// let max_channels = 8;
    /// let max_samplerate = 96000;
    ///
    /// let mut limiter = PcmLimiter::new(max_attack_ms, max_channels, max_samplerate).unwrap();
    /// let thresh_val = 0.85; // threshold value
    /// limiter.set_threshold(thresh_val);
    ///
    /// assert!(limiter.set_threshold(thresh_val) == Ok(()));
    /// assert!(limiter.set_threshold(thresh_val * -1.0) == Err(TdLimiterError::InvalidParameter));
    /// assert!(limiter.set_threshold(thresh_val * 2.0) == Err(TdLimiterError::InvalidParameter));
    /// ```
    pub fn set_threshold(&mut self, threshold: f32) -> Result<(), TdLimiterError> {
        if !(0.0f32..=1.0f32).contains(&threshold) {
            return Err(TdLimiterError::InvalidParameter);
        }
        self.threshold = threshold;
        Ok(())
    }
}
