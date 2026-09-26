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
//! Decoding of long and short blocks

use std::f32::consts::{FRAC_1_SQRT_2, PI};

use crate::aac_dec::lpd::common::LpdMode;
use crate::aac_dec::lpd::constants::*;
use crate::aac_dec::lpd::fac::FacData;
use crate::aac_dec::lpd::{bass_postfilter, lpc as usac_lpc};
use crate::aac_dec::{
    channel_info::IcsInfo, constants, error_codes::AacDecoderError, hcr::HcrData,
    huff_dec::HuffmanDecoder, lpd::LpdData, pns, pns::PnsData, utils::*,
};
use crate::arith_coding::arith_dec::ArithDecoderData;
use crate::common::bitstream::Bitstream;
use crate::common::flags::ACFlags;
use crate::common::mdct::Mdct;
use crate::common::tables::window_tables;
use itertools::izip;

use super::lpd::UsacCoremode;

/// ISO/IEC 23003-3, 7.11.2.6 Modification of core decoder output (pseudo LR).
pub fn apply_pseudo_lr(time_data_left: &mut [f32], time_data_right: &mut [f32]) {
    for (l, r) in izip!(time_data_left.iter_mut(), time_data_right.iter_mut()) {
        let accu = (*l + *r) * FRAC_1_SQRT_2;
        *r = (*l - *r) * FRAC_1_SQRT_2;
        *l = accu
    }
}

/// Transform MDCT spectral data into time domain.
///
/// # Parameters
///
/// - `ics_info`: Individual channel stream info with valid internal data.
/// - `lpd_data_opt`: Optional reference to LPD data struct.
/// - `imdct`: MDCT instance.
/// - `spectrum`: Spectral coefficients.
/// - `out_samples`: Output data in time domain.
/// - `work_buffer`: Work buffer.
/// - `is_frame_ok`: Indicates that last from was OK.
/// - `ac_flags`: Flags to provide information about audio codec.
#[expect(clippy::too_many_arguments)]
pub fn frequency_to_time(
    ics_info: &IcsInfo,
    mut lpd_data_opt: Option<&mut LpdData>,
    imdct: &mut Mdct,
    spectrum: &mut [f32],
    out_samples: &mut [f32],
    work_buffer: &mut [f32],
    is_frame_ok: bool,
    ac_flags: ACFlags,
) {
    let n_samples_written;

    let frame_len = spectrum.len();

    // Fill pitch and pitch gain with zeros to prevent Ferret errors in
    // `bass_postfilter::filter_1sf_delay()`.
    let mut pitch = [0u16; NB_SUBFR_SUPERFR + SYN_SFD];
    let mut pit_gain = [0.0f32; NB_SUBFR_SUPERFR + SYN_SFD];

    let (tl, fl, fr, n_spec) = ics_info.get_window_params(frame_len, imdct.prev_tl);

    let is_lpd_last_core_mode = if let Some(lpd_data) = lpd_data_opt.as_deref_mut() {
        lpd_data.last_core_mode == UsacCoremode::Lpd
    } else {
        false
    };

    if is_lpd_last_core_mode {
        let lpd_data = lpd_data_opt.as_mut().unwrap();
        debug_assert!(ac_flags.contains(ACFlags::USAC));

        let last_lpd_mode = lpd_data.last_lpd_mode;

        if lpd_data.last_lpd_mode.is_time_domain() {
            // ACELP (or TCX concealment) --> FD

            // Keep some free space at the beginning of the buffer. To be used for past data.
            let synth = &mut work_buffer[PIT_MAX_MAX - L_SUBFR..];
            let a = &mut lpd_data.lpd_info.lp_coeffs[0];
            {
                let mut lsp_coeffs: [f32; M_LP_FILTER_ORDER] = Default::default();
                for (lsp, lpc) in
                    izip!(lsp_coeffs.iter_mut(), lpd_data.lpc4_lsf.iter()).take(M_LP_FILTER_ORDER)
                {
                    *lsp = (*lpc * PI / 6400.0f32).cos();
                }
                usac_lpc::lsp_to_lpc(&lsp_coeffs, a);
            }

            // Get FAC (incl. FAC ZIR, hence the '*2') length.
            lpd_data.fac_len = FacData::fac_len(frame_len, n_spec > 1) * 2;

            lpd_data.fac_offset = 0;
            let fac_w_zir_opt =
                { Some(&mut lpd_data.old_exc_mem[lpd_data.fac_offset..lpd_data.fac_len]) };

            let n_samples_read_fac;
            (n_samples_written, n_samples_read_fac) = lpd_data.fac_data.acelp2mdct(
                spectrum,
                &mut synth[..frame_len],
                0,
                fac_w_zir_opt,
                a,
                imdct,
                &mut lpd_data.acelp_data,
                window_tables::get_table(fr as u16, ics_info.window_shape()).unwrap(),
                0.0, // FAC gain has already been applied.
                0,
                tl,
                last_lpd_mode,
                n_spec > 1,
                lpd_data.was_last_lpc_lost || !is_frame_ok,
                true,
            );

            lpd_data.fac_offset += n_samples_read_fac;
            if lpd_data.fac_offset >= lpd_data.fac_len {
                lpd_data.fac_offset = 0;
                lpd_data.fac_len = 0;
            }
        } else {
            // TCX --> FD

            let synth = &mut work_buffer[PIT_MAX_MAX - L_SUBFR..];
            let fac_w_zir_opt = if lpd_data.fac_len > lpd_data.fac_offset {
                Some(&lpd_data.old_exc_mem[lpd_data.fac_offset..lpd_data.fac_len])
            } else {
                None
            };

            //let n_elements_read_fac;
            (n_samples_written, _) = imdct.imlt(
                &mut synth[..frame_len],
                spectrum,
                n_spec,
                &fac_w_zir_opt,
                window_tables::get_table(fl as u16, ics_info.window_shape()).unwrap(),
                window_tables::get_table(fr as u16, ics_info.window_shape()).unwrap(),
                1.0,
            );

            // FAC buffer must have been drained by now.
            lpd_data.fac_len = 0;
            lpd_data.fac_offset = 0;
        }

        {
            // Number of subframes per division.
            let lpd_sfd = (frame_len / L_SUBFR) >> 1;
            let syn_sfd = lpd_sfd - BPF_SFD;

            pitch[..syn_sfd].copy_from_slice(&(lpd_data.old_t_pf[..syn_sfd]));
            pitch[syn_sfd..lpd_sfd + 3].fill(L_SUBFR as u16);

            pit_gain[..syn_sfd].copy_from_slice(&(lpd_data.old_gain_pf[..syn_sfd]));
            pit_gain[syn_sfd..lpd_sfd + 3].fill(0.0);

            if lpd_data.last_lpd_mode.is_acelp() {
                pitch[syn_sfd] = pitch[syn_sfd - 1];
                pit_gain[syn_sfd] = pit_gain[syn_sfd - 1];
                if ics_info.is_long_block() {
                    pitch[syn_sfd + 1] = pitch[syn_sfd];
                    pit_gain[syn_sfd + 1] = pit_gain[syn_sfd];
                }
            }

            // Copy old data to the beginning of the buffer.
            work_buffer[..PIT_MAX_MAX - L_SUBFR]
                .copy_from_slice(&lpd_data.old_synth[..PIT_MAX_MAX - L_SUBFR]);
            {
                // Recalculate pitch gain to allow postfilering on FAC area.
                for (i, (t, gain)) in izip!(&pitch, &mut pit_gain).take(syn_sfd + 2).enumerate() {
                    if *gain > 0.0f32 {
                        *gain = get_gain(
                            &work_buffer[PIT_MAX_MAX + i * L_SUBFR..],
                            &work_buffer[PIT_MAX_MAX + (i * L_SUBFR) - *t as usize..],
                            L_SUBFR,
                        );
                    }
                }

                bass_postfilter::filter_1sf_delay(
                    work_buffer,
                    &mut out_samples[..frame_len],
                    &pitch,
                    &pit_gain,
                    (lpd_sfd + 2) * L_SUBFR + BPF_SFD * L_SUBFR,
                    frame_len - (lpd_sfd + 4) * L_SUBFR,
                    &mut lpd_data.mem_bpf,
                );
            }
        }
    } else {
        // FD --> FD.

        (n_samples_written, _) = imdct.imlt(
            &mut out_samples[..frame_len],
            spectrum,
            n_spec,
            &None,
            window_tables::get_table(fl as u16, ics_info.window_shape()).unwrap(),
            window_tables::get_table(fr as u16, ics_info.window_shape()).unwrap(),
            1.0,
        );
    }

    if ac_flags.contains(ACFlags::USAC) {
        let lpd_data = lpd_data_opt.unwrap();
        lpd_data.last_core_mode = if ics_info.is_long_block() {
            UsacCoremode::FdLong
        } else {
            UsacCoremode::FdShort
        };
        lpd_data.last_lpd_mode = LpdMode::NotLpd;
    }
    debug_assert!(n_samples_written == frame_len);
}

/// Initializes codebook data
///
/// Depending on `ics_info` the `code_book` slice gets initialized with escape
/// codebook `CBTYPE_ESCBOOK`. Codebooks above signaled scale factor bands get
/// initialized with zero codebook `CBTYPE_ZERO_HCB`.
/// This initialization might be required for codec configurations which do not
/// include `section_data` bitstream field.
///
/// # Parameters
///
/// - `ics_info`: Individual channel stream info data
/// - `code_book`: Code book description for each scale factor band filled on return
///
/// # Examples
///
/// ```
/// use aac::aac_dec::{block::init_code_book_data, channel_info::IcsInfo, constants};
///
/// let ics_info = IcsInfo::new();
/// let mut code_book = vec![0_u8; constants::MAX_WINS_X_SFBS];
///
/// init_code_book_data(&ics_info, &mut code_book);
/// ```
pub fn init_code_book_data(ics_info: &IcsInfo, code_book: &mut [u8]) {
    for code_book in
        code_book.chunks_exact_mut(constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame())
    {
        code_book[..ics_info.max_sf_bands()].fill(CBTYPE_ESCBOOK);
        code_book[ics_info.max_sf_bands()..].fill(CBTYPE_ZERO_HCB);
    }
}

/// Manages arithmetic decoding in the frequency domain (FD) path.
///
/// # Parameters
///
/// - `bs`: Bitstream instance with valid internal data
/// - `ics_info`: Individual channel stream info data
/// - `spectrum`: Quantized spectral coefficients output
/// - `ac_flags`: Flags to provide information about audio codec. Used to check if the independent
///   flag is set.
///
/// Returns `AacDecoderError` type
/// - `AacDecOk` on success
/// - `AacDecParseError` on failure
pub fn read_ac_spectral_data(
    bs: &mut Bitstream,
    arco_data: &mut ArithDecoderData,
    ics_info: &IcsInfo,
    spectrum: &mut [i16],
    ac_flags: ACFlags,
) -> AacDecoderError {
    let mut error_aac = AacDecoderError::Ok;
    let mut is_decode_successful;

    // number of transmitted spectral coefficients
    let scale_factor_bands_offsets = ics_info.scale_factor_bands();
    let lg: u16 = scale_factor_bands_offsets[ics_info.max_sf_bands()];
    let window_length = spectrum.len() / ics_info.windows_per_frame();

    let arith_reset_flag = if ac_flags.contains(ACFlags::INDEP) {
        true
    } else {
        bs.read_bit() == 1
    };
    for (win_num, win_spec) in spectrum.chunks_exact_mut(window_length).enumerate() {
        is_decode_successful = arco_data.decode(
            bs,
            win_spec,
            usize::from(lg),
            window_length,
            arith_reset_flag && (win_num == 0),
        );

        if !is_decode_successful {
            error_aac = AacDecoderError::ParseError;
            break;
        }
    }

    error_aac
}

/// Reads section data
///
/// Reads the codebooks from `Bitstream` and its associated section length. For every scale
/// factor band the provided `code_book` slice gets filled on return.
/// While parsing the bitstream payload, `VCB11` extra treatment is considered, and `HCR`
/// related `hcr_data` may be updated.
///
/// # Parameters
///
/// - `bs`: Bitstream data to read from
/// - `ics_info`: Individual channel stream info data
/// - `hcr_data`: Huffman codebook reordering struct might be updated on return
/// - `code_book`: Code book description for each scale factor band filled on return
/// - `channel`: Channel to be processed in present element `[0; 1]`
/// - `common_window`: Common windows signals whether stereo coding is possible
/// - `ac_flags`: Flags to provide codec related information
///
/// # Errors
///
/// Returns `AacDecoderError` type
/// - `AacDecOk` on success
/// - `AacDecInvalidCodeBook`, `AacDecDecodeFrameError`, `AacDecParseError` on failure
///
/// # Examples
///
/// ```
/// use aac::aac_dec::{block::read_section_data, channel_info::IcsInfo, constants, hcr::HcrData};
/// use aac::common::{
///     bitstream::{Bitstream, Mode},
///     flags::ACFlags,
/// };
///
/// let bit_buffer = vec![0; 8];
/// let mut bs_reader = Bitstream::new(bit_buffer.len(), Mode::Reader);
/// bs_reader.init(&bit_buffer, 64);
/// let ics_info = IcsInfo::new();
/// let mut hcr_data = HcrData::default();
/// let mut code_book = vec![0_u8; constants::MAX_WINS_X_SFBS];
/// let channel = 0;
/// let common_window = true;
/// let ac_flags = ACFlags::empty();
///
/// let error_status = read_section_data(
///     &mut bs_reader,
///     &ics_info,
///     Some(&mut hcr_data),
///     &mut code_book,
///     channel,
///     common_window,
///     ac_flags,
/// );
/// ```
pub fn read_section_data(
    bs: &mut Bitstream,
    ics_info: &IcsInfo,
    mut hcr_data: Option<&mut HcrData>,
    code_book: &mut [u8],
    channel: usize,
    common_window: bool,
    ac_flags: ACFlags,
) -> AacDecoderError {
    let sf_bands = ics_info.scale_factor_bands();
    let n_bits = if ics_info.is_long_block() { 5 } else { 3 };
    let sect_esc_val = (1 << n_bits) - 1;

    if ac_flags.contains(ACFlags::ER_HCR) {
        if let Some(hcr) = &mut hcr_data {
            hcr.set_number_section(channel, 0);
        }
    }

    for code_book in code_book
        .chunks_exact_mut(constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame())
        .take(ics_info.n_window_groups())
    {
        let mut band = 0;
        while band < ics_info.max_sf_bands() {
            let mut sect_len = 0;

            let sect_cb = if ac_flags.contains(ACFlags::ER_VCB11) {
                bs.read(5) as u8
            } else {
                bs.read(4) as u8
            };

            if !ac_flags.contains(ACFlags::ER_VCB11) || ((sect_cb != 11) && (sect_cb < 16)) {
                let mut sect_len_incr = bs.read(n_bits) as usize;
                while sect_len_incr == sect_esc_val {
                    sect_len += sect_esc_val;
                    sect_len_incr = bs.read(n_bits) as usize;
                }
                sect_len += sect_len_incr;
            } else {
                sect_len += 1;
            }

            if ac_flags.contains(ACFlags::ER_HCR) {
                if (band + sect_len) > ics_info.n_total_sf_bands() {
                    return AacDecoderError::DecodeFrameError;
                }
                if let Some(hcr) = &mut hcr_data {
                    let number_section = hcr.get_number_section(channel);
                    let error = hcr.set_codebook_and_num_lines_in_section(
                        channel,
                        sect_cb,
                        sf_bands[band + sect_len] - sf_bands[band],
                    );
                    if error != AacDecoderError::Ok {
                        return error;
                    }
                    hcr.set_number_section(channel, number_section + 1);
                }
            } else if ac_flags.contains(ACFlags::ER_RVLC) {
                sect_len = sect_len.min(code_book.len() - band);
            } else if (band + sect_len) > code_book.len() {
                return AacDecoderError::DecodeFrameError;
            }

            // Check if decoded codebook index is feasible
            if (sect_cb == CBTYPE_BOOKSCL)
                || ((sect_cb == CBTYPE_INTENSITY_HCB2 || sect_cb == CBTYPE_INTENSITY_HCB)
                    && !(common_window || ac_flags.contains(ACFlags::ER_RVLC)))
            {
                return AacDecoderError::InvalidCodeBook;
            }

            // Store codebook index
            code_book[band..band + sect_len].fill(sect_cb);
            band += sect_len;
        }
        code_book[band..].fill(CBTYPE_ZERO_HCB);
    }

    AacDecoderError::Ok
}

/// Reads scale factor data
///
/// Differential coded scale factors are read from `Bitstream` and fully decoded
/// with `HuffmanDecoder` by considering the `code_book` and additional side info.
/// Depending on the `code_book` the scale factors can also be used for intensity stereo
/// coding or perceptual noise substitution.
///
/// # Parameters
///
/// - `bs`: Bitstream data to read from
/// - `ics_info`: Individual channel stream info data
/// - `global_gain`: Global gain of the quantized spectrum
/// - `code_book`: Code book description for each scale factor band
/// - `scale_factor`: Scale factor data filled on return
/// - `pns_data`: Perceptual noise substitution related data updated on return
/// - `ac_flags`: Flags to provide codec related information
///
/// # Errors
///
/// Returns `AacDecoderError` type
/// - `AacDecOk` on success
/// - `AacDecParseError` on failure
///
/// # Examples
///
/// ```
/// use aac::aac_dec::{
///     block::read_scalefactor_data, channel_info::IcsInfo, constants, pns::PnsData,
/// };
/// use aac::common::{
///     bitstream::{Bitstream, Mode},
///     flags::ACFlags,
/// };
///
/// let bit_buffer = vec![0; 8];
/// let mut bs_reader = Bitstream::new(bit_buffer.len(), Mode::Reader);
/// bs_reader.init(&bit_buffer, 64);
/// let ics_info = IcsInfo::new();
/// let global_gain = 0_i16;
/// let code_book = vec![0_u8; constants::MAX_WINS_X_SFBS];
/// let mut scale_factor = vec![0_i16; constants::MAX_WINS_X_SFBS];
/// let mut pns_data = PnsData::default();
/// let ac_flags = ACFlags::empty();
///
/// let error_status = read_scalefactor_data(
///     &mut bs_reader,
///     &ics_info,
///     global_gain,
///     &code_book,
///     &mut scale_factor,
///     Some(&mut pns_data),
///     ac_flags,
/// );
/// ```
pub fn read_scalefactor_data(
    bs: &mut Bitstream,
    ics_info: &IcsInfo,
    global_gain: i16,
    code_book: &[u8],
    scale_factor: &mut [i16],
    mut pns_data: Option<&mut PnsData>,
    ac_flags: ACFlags,
) -> AacDecoderError {
    let huff_dec = HuffmanDecoder::new(CBTYPE_BOOKSCL);
    let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
    let mut factor = global_gain;
    let mut pns_nrg = global_gain - pns::NOISE_OFFSET - 256;
    let mut is_position = 0;

    for (group, (code_book, scale_factor)) in izip!(
        code_book.chunks_exact(bands_per_window),
        scale_factor.chunks_exact_mut(bands_per_window),
    )
    .take(ics_info.n_window_groups())
    .enumerate()
    {
        for band in 0..ics_info.max_sf_bands() {
            match code_book[band] {
                CBTYPE_ZERO_HCB => {
                    scale_factor[band] = 0;
                }
                CBTYPE_INTENSITY_HCB2 | CBTYPE_INTENSITY_HCB => {
                    let temp = huff_dec.read_word(bs);
                    is_position += temp - 60;
                    if !(-287..=287).contains(&is_position) {
                        return AacDecoderError::ParseError;
                    }
                    scale_factor[band] = is_position;
                }
                CBTYPE_NOISE_HCB => {
                    if ac_flags.contains(ACFlags::USAC) {
                        return AacDecoderError::ParseError;
                    }
                    let pns = pns_data.as_mut().unwrap();
                    let temp = if pns.is_pns_active() {
                        huff_dec.read_word(bs) - 60
                    } else {
                        pns.set_is_pns_active(true);
                        bs.read(9) as i16
                    };
                    pns_nrg = pns::clamp_noise_range(pns_nrg + temp);
                    scale_factor[band] = pns_nrg;
                    pns.is_pns_used_mut()[group * bands_per_window + band] = true;
                }
                _ => {
                    if !(ac_flags.contains(ACFlags::USAC) && group == 0 && band == 0) {
                        let temp = huff_dec.read_word(bs);
                        factor += temp - 60;
                    }
                    if !(0..=255).contains(&factor) {
                        return AacDecoderError::ParseError;
                    }
                    scale_factor[band] = factor - 100;
                }
            }
        }
    }

    AacDecoderError::Ok
}

/// Reads huffman coded spectral lines
///
/// The `HuffmanDecoder` reads all huffman coded spectral lines from `Bitstream` by using
/// signaled `code_book` and its associated huffman table. The whole provided `spectrum` buffer
/// gets filled on return.
///
/// # Parameters
///
/// - `bs`: Bitstream data to read from
/// - `ics_info`: Individual channel stream info data
/// - `code_book`: Code book description for each scale factor band
/// - `spectrum`: Spectral data buffer filled on return
///
/// # Errors
///
/// Returns `AacDecoderError` type
/// - `AacDecOk` on success
///
/// # Examples
///
/// ```
/// use aac::aac_dec::{block::read_spectral_data, channel_info::IcsInfo, constants};
/// use aac::common::bitstream::{Bitstream, Mode};
///
/// let bit_buffer = vec![0; 8];
/// let mut bs_reader = Bitstream::new(bit_buffer.len(), Mode::Reader);
/// bs_reader.init(&bit_buffer, 64);
/// let ics_info = IcsInfo::new();
/// let code_book = vec![0_u8; constants::MAX_WINS_X_SFBS];
/// let mut spectrum = vec![0_i16; constants::MAX_FRAMESIZE];
///
/// let error_status = read_spectral_data(&mut bs_reader, &ics_info, &code_book, &mut spectrum);
/// ```
pub fn read_spectral_data(
    bs: &mut Bitstream,
    ics_info: &IcsInfo,
    code_book: &[u8],
    spectrum: &mut [i16],
) -> AacDecoderError {
    let sf_bands = ics_info.scale_factor_bands();
    let window_length = spectrum.len() / ics_info.windows_per_frame();
    let mut group_offset = 0;

    for (code_book, windows_per_group) in izip!(
        code_book.chunks_exact(constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame()),
        ics_info.window_group_lengths().iter()
    )
    .take(ics_info.n_window_groups())
    {
        let mut stop_line = usize::from(sf_bands[0]);
        let mut band = 0;

        while band < ics_info.max_sf_bands() {
            // Group contiguous bands with same code book
            let mut current_cb = code_book[band];
            loop {
                band += 1;
                if !ics_info.is_long_block()
                    || band >= (ics_info.max_sf_bands())
                    || code_book[band] != current_cb
                {
                    break;
                }
            }
            let start_line = stop_line;
            stop_line = usize::from(sf_bands[band]);

            if (16..=31).contains(&current_cb) {
                current_cb = CBTYPE_ESCBOOK; // VCB11 has to use escape codebook
            }
            let huffman_decoder = if (1..=12).contains(&current_cb) {
                Some(HuffmanDecoder::new(current_cb))
            } else {
                None
            };

            for spectrum in spectrum[group_offset * window_length..]
                .chunks_exact_mut(window_length)
                .take((*windows_per_group).into())
            {
                if let Some(huff_dec) = huffman_decoder {
                    huff_dec.read_buf(bs, &mut spectrum[start_line..stop_line]);
                } else {
                    spectrum[start_line..stop_line].fill(0);
                };
            }
        }
        group_offset += usize::from(*windows_per_group);
    }

    AacDecoderError::Ok
}
