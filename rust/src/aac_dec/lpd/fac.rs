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
//! USAC forward aliasing cancellation (FAC) tool
//!
//! The FAC tool cancels time-domain aliasing for transitions between ACELP
//! and transform coded frames.

use arrayvec::ArrayVec;
use itertools::izip;
use num_complex::Complex;
use std::cmp::Ordering;

use crate::aac_dec::constants::*;
use crate::aac_dec::lpd::acelp;
use crate::aac_dec::lpd::common as lpd_common;
use crate::aac_dec::lpd::common::LpdMode;
use crate::aac_dec::lpd::constants::*;
use crate::aac_dec::lpd::lpc;
use crate::common::bitstream::Bitstream;
use crate::common::dct;
use crate::common::dct::dctiv;
use crate::common::enums::WindowShape;
use crate::common::mdct;
use crate::common::tables;

use super::tcx;

// TABLES
// FAC window tables for coreCoderFrameLength = 1024.
#[rustfmt::skip]
static FAC_WINDOW_SYNTH_128: [f32; 128] =
[
  0.499990553,0.499915272,0.499764711,0.499538898,0.499237806,0.498861521,0.498410136,0.497883707,
  0.497282296,0.496605992,0.495854884,0.495029122,0.494128793,0.493154019,0.492105007,0.490981936,
  0.489784896,0.488514066,0.487169683,0.485751927,0.484261036,0.482697219,0.481060743,0.479351729,
  0.477570593,0.475717515,0.473792791,0.471796781,0.469729573,0.467591733,0.465383470,0.463105112,
  0.460757047,0.458339542,0.455853015,0.453297883,0.450674385,0.447983146,0.445224345,0.442398548,
  0.439506084,0.436547518,0.433523118,0.430433452,0.427278996,0.424060166,0.420777500,0.417431414,
  0.414022535,0.410551280,0.407018155,0.403423786,0.399768651,0.396053284,0.392278314,0.388444245,
  0.384551674,0.380601227,0.376593411,0.372528881,0.368408293,0.364232212,0.360001266,0.355716079,
  0.351377368,0.346985728,0.342541814,0.338046342,0.333499938,0.328903347,0.324257195,0.319562197,
  0.314819127,0.310028613,0.305191427,0.300308257,0.295379847,0.290406972,0.285390377,0.280330777,
  0.275229007,0.270085722,0.264901817,0.259678006,0.254415065,0.249113828,0.243775070,0.238399625,
  0.232988253,0.227541789,0.222061068,0.216546923,0.211000144,0.205421582,0.199812099,0.194172516,
  0.188503698,0.182806507,0.177081764,0.171330348,0.165553153,0.159751013,0.153924823,0.148075432,
  0.142203763,0.136310682,0.130397052,0.124463797,0.118511796,0.112541959,0.106555156,0.100552313,
  0.094534338,0.088502109,0.082456559,0.076398589,0.070329115,0.064249054,0.058159314,0.052060816,
  0.045954477,0.039841220,0.033721957,0.027597627,0.021469127,0.015337405,0.009203359,0.003067946
];

#[rustfmt::skip]
static FAC_WINDOW_ZIR_128: [f32; 128] =
[
  0.496932089,0.490796685,0.484662592,0.478530854,0.472402334,0.466278076,0.460158765,0.454045534,
  0.447939187,0.441840708,0.435750902,0.429670841,0.423601359,0.417543441,0.411497921,0.405465662,
  0.399447650,0.393444836,0.387458056,0.381488204,0.375536203,0.369602948,0.363689274,0.357796252,
  0.351924509,0.346075207,0.340248942,0.334446818,0.328669667,0.322918296,0.317193508,0.311496347,
  0.305827469,0.300187886,0.294578373,0.288999856,0.283453137,0.277938932,0.272458225,0.267011732,
  0.261600405,0.256224930,0.250886172,0.245584965,0.240322009,0.235098213,0.229914248,0.224771038,
  0.219669163,0.214609608,0.209593058,0.204620168,0.199691743,0.194808632,0.189971402,0.185180902,
  0.180437803,0.175742745,0.171096683,0.166500032,0.161953628,0.157458127,0.153014213,0.148622647,
  0.144283891,0.139998794,0.135767817,0.131591678,0.127471164,0.123406641,0.119398780,0.115448385,
  0.111555785,0.107721746,0.103946708,0.100231327,0.096576244,0.092981867,0.089448728,0.085977443,
  0.082568519,0.079222575,0.075939864,0.072721004,0.069566570,0.066476919,0.063452527,0.060493845,
  0.057601437,0.054775633,0.052016862,0.049325556,0.046702135,0.044147007,0.041660469,0.039243028,
  0.036894843,0.034616530,0.032408230,0.030270426,0.028203242,0.026207261,0.024282478,0.022429463,
  0.020648303,0.018939322,0.017302828,0.015739009,0.014248038,0.012830322,0.011485903,0.010215168,
  0.009018025,0.007894965,0.006845996,0.005871236,0.004970914,0.004145132,0.003393984,0.002717673,
  0.002116275,0.001589858,0.001138482,0.000762198,0.000461168,0.000235305,0.000084756,0.000009418
];

#[rustfmt::skip]
static FAC_WINDOW_SYNTH_64: [f32; 64] =
[
  0.499962360,0.499661207,0.499059021,0.498156816,0.496953696,0.495451689,0.493650109,0.491552174,
  0.489158213,0.486470044,0.483488619,0.480215520,0.476653516,0.472803503,0.468669981,0.464253336,
  0.459557027,0.454584420,0.449337333,0.443819821,0.438034981,0.431986600,0.425677449,0.419112056,
  0.412295043,0.405229092,0.397918195,0.390368849,0.382583499,0.374568015,0.366326928,0.357865334,
  0.349187642,0.340300530,0.331208497,0.321915954,0.312429309,0.302755624,0.292899281,0.282865971,
  0.272662789,0.262294412,0.251768976,0.241092250,0.230269745,0.219308123,0.208214447,0.196996421,
  0.185658947,0.174209103,0.162655011,0.151002854,0.139259592,0.127432749,0.115529425,0.103556097,
  0.091519944,0.079428606,0.067290425,0.055110883,0.042898413,0.030660551,0.018403890,0.006135883
];

#[rustfmt::skip]
static FAC_WINDOW_ZIR_64: [f32; 64] =
[ 0.493864536,0.481595665,0.469340175,0.457100719,0.444888562,0.432709336,0.420571536,0.408480585,
  0.396445006,0.384470344,0.372566521,0.360740572,0.348996311,0.337345362,0.325789899,0.314341456,
  0.303003758,0.291785181,0.280692518,0.269730628,0.258907974,0.248231232,0.237705156,0.227337927,
  0.217134312,0.207100600,0.197244942,0.187569961,0.178083688,0.168792218,0.159699559,0.150812000,
  0.142135069,0.133672789,0.125430882,0.117416739,0.109632201,0.102080770,0.094771385,0.087705657,
  0.080888592,0.074323162,0.068014383,0.061965112,0.056180030,0.050663497,0.045416262,0.040442672,
  0.035746720,0.031330649,0.027196571,0.023346117,0.019785147,0.016511261,0.013529954,0.010840441,
  0.008448087,0.006349941,0.004548849,0.003045621,0.001843198,0.000941770,0.000337930,0.000038027
];

// FAC window tables for coreCoderFrameLength = 768
#[rustfmt::skip]
static FAC_WINDOW_SYNTH_96: [f32; 96] =
[
  0.499982774,0.499849379,0.499581188,0.499180406,0.498645037,0.497976542,0.497174442,0.496239632,
  0.495172322,0.493971288,0.492639005,0.491174251,0.489578247,0.487850517,0.485992998,0.484006077,
  0.481888056,0.479641646,0.477267236,0.474764109,0.472134739,0.469377995,0.466496408,0.463489860,
  0.460358471,0.457105041,0.453728348,0.450230449,0.446612328,0.442873836,0.439017564,0.435043246,
  0.430952460,0.426747084,0.422426939,0.417993337,0.413448036,0.408792853,0.404026866,0.399153799,
  0.394172817,0.389087081,0.383897722,0.378604829,0.373210400,0.367715955,0.362123579,0.356433451,
  0.350648999,0.344770074,0.338799655,0.332737714,0.326586723,0.320348501,0.314024329,0.307615399,
  0.301124990,0.294553995,0.287904143,0.281177312,0.274374902,0.267499238,0.260551423,0.253534079,
  0.246449307,0.239297852,0.232082784,0.224805579,0.217468679,0.210072324,0.202620223,0.195114791,
  0.187556401,0.179947540,0.172290891,0.164587811,0.156840667,0.149052322,0.141223073,0.133356720,
  0.125453457,0.117518239,0.109550409,0.101553597,0.093529478,0.085480660,0.077408597,0.069316059,
  0.061205596,0.053077843,0.044936478,0.036782045,0.028618261,0.020447725,0.012271080,0.004090968
];

#[rustfmt::skip]
static FAC_WINDOW_ZIR_96: [f32; 96] =
[
  0.495909929,0.487729102,0.479553193,0.471380860,0.463218153,0.455063730,0.446922243,0.438795298,
  0.430682927,0.422591537,0.414519459,0.406470001,0.398446172,0.390449762,0.382482231,0.374545187,
  0.366643488,0.358776987,0.350947380,0.343159467,0.335411489,0.327709883,0.320052952,0.312443912,
  0.304885954,0.297378838,0.289927512,0.282531738,0.275194645,0.267917842,0.260702699,0.253550619,
  0.246466681,0.239448562,0.232501313,0.225624666,0.218823329,0.212095231,0.205445290,0.198874742,
  0.192385018,0.185975611,0.179651320,0.173413545,0.167263433,0.161200464,0.155229464,0.149351642,
  0.143566415,0.137876570,0.132283509,0.126790106,0.121395588,0.116102919,0.110913172,0.105827391,
  0.100846589,0.095973693,0.091207631,0.086551532,0.082006089,0.077572331,0.073252991,0.069047093,
  0.064957440,0.060982779,0.057125978,0.053387702,0.049768813,0.046272017,0.042895805,0.039641201,
  0.036510494,0.033504553,0.030621864,0.027865117,0.025236752,0.022733293,0.020359250,0.018111205,
  0.015995514,0.014006575,0.012148925,0.010422705,0.008826481,0.007360454,0.006028836,0.004828214,
  0.003760491,0.002826003,0.002023030,0.001355546,0.000819873,0.000417904,0.000149960,0.000015974
];

#[rustfmt::skip]
static FAC_WINDOW_SYNTH_48: [f32; 48] =
[
  0.499932885,0.499397963,0.498327285,0.496723861,0.494588494,0.491923183,0.488731265,0.485015661,
  0.480781198,0.476031065,0.470771968,0.465008885,0.458747029,0.451995194,0.444758296,0.437044919,
  0.428864151,0.420224041,0.411134094,0.401603341,0.391642898,0.381263524,0.370475382,0.359290868,
  0.347720951,0.335779518,0.323477477,0.310830712,0.297849834,0.284550101,0.270945638,0.257051587,
  0.242881700,0.228451937,0.213777274,0.198874414,0.183758199,0.168445110,0.152951360,0.137294352,
  0.121490464,0.105556220,0.089508295,0.073365636,0.057143189,0.040860768,0.024533613,0.008180730
];

#[rustfmt::skip]
static FAC_WINDOW_ZIR_48: [f32; 48] =
[
  0.491819263,0.475466311,0.459139735,0.442857146,0.426634520,0.410490811,0.394443661,0.378510594,
  0.362705946,0.347048789,0.331554770,0.316241324,0.301125795,0.286221713,0.271548003,0.257118046,
  0.242948562,0.229054525,0.215450436,0.202150881,0.189169958,0.176521808,0.164220035,0.152278334,
  0.140709922,0.129524067,0.118737191,0.108356051,0.098396614,0.088866785,0.079776138,0.071135834,
  0.062954664,0.055241253,0.048005629,0.041253492,0.034990486,0.029227242,0.023968641,0.019218786,
  0.014983489,0.011268046,0.008077575,0.005412626,0.003275331,0.001673355,0.000601916,0.000066041
];

#[repr(C)]
#[derive(Debug)]
pub struct FacData {
    data_0: [f32; LFAC],
    data_index: [i8; N_MODS],
    mem_state: u8,
}

impl Default for FacData {
    fn default() -> Self {
        Self {
            data_0: [0.0f32; LFAC],
            data_index: [-1; N_MODS],
            mem_state: 0,
        }
    }
}

impl FacData {
    pub fn change_mdct(&mut self, mdct: &mut mdct::Mdct, wrs: &'static [Complex<f32>]) {
        mdct.prev_wrs = Some(wrs);
    }

    pub fn change_tcx(&mut self, tcx: &mut tcx::TcxData) {
        tcx.last_pitch = 0;
    }

    /// Does FAC transition from ACELP domain to frequency domain.
    ///
    /// # Parameters
    ///
    /// - `data_in`: ACELP input.
    /// - `data_out`: Time domain output.
    /// - `data_out_offset`: Offset to write output data.
    /// - `fac_w_zir_opt`: Optional FAC buffer, incl. FAC ZIR.
    /// - `fac_zir_offset`: Offset to FAC data.
    /// - `a`: LP domain filter coefficients.
    /// - `mdct`: MDCT instance.
    /// - `acelp_data`: ACELP data.
    /// - `wrs`: Right MDCT window slope.
    /// - `gain`: Gain to be applied to FAC data before overlap add.
    /// - `k`: Index of current subframe.
    /// - `tl`: MDCT transform length of `data_in` (should be <  `data_in.len()`).
    /// - `last_lpd_mode`: Last LPD mode.
    /// - `is_short`: Is MDCT frame short blocks?
    /// - `last_frame_lost`: Was last frame lost?
    /// - `is_fd_fac`: Indicates FAC processing to FD.
    ///
    /// # Return
    /// - (`written samples`, `read_samples_fac`)
    #[expect(clippy::too_many_arguments)]
    pub fn acelp2mdct(
        &mut self,
        data_in: &mut [f32],
        data_out: &mut [f32],
        data_out_offset: usize,
        fac_w_zir_opt: Option<&mut [f32]>,
        a: &[f32; M_LP_FILTER_ORDER],
        mdct: &mut mdct::Mdct,
        acelp_data: &mut acelp::AcelpData,
        wrs: &'static [Complex<f32>],
        gain: f32,
        k: usize,
        tl: usize,
        last_lpd_mode: LpdMode,
        is_short: bool,
        last_frame_lost: bool,
        is_fd_fac: bool,
    ) -> (usize, usize) {
        let mut n_samples_written_out: usize = 0; // Samples written to output buffer.
        let mut n_samples_read_fac: usize = 0; // Bytes read from FAC ZIR buffer.
        let mut ovl_offset = mdct.ov_offset; // Samples written to overlap buffer.

        let ovl_len = mdct.overlap.len();

        // Derive length information
        let fac_len = FacData::fac_len(data_in.len(), is_short);
        let output_len = data_out.len() - data_out_offset;
        let mut fl: usize = fac_len * 2;
        let fr: usize = wrs.len() * 2;
        let mut nl: usize = (tl - fl) >> 1;
        let nr: usize = (tl - fr) >> 1;

        // Set window table
        debug_assert!(&[48, 64, 96, 128].contains(&fac_len));
        let window_table: &[Complex<f32>] = match fac_len {
            128 => &tables::window_tables::SINE_WINDOW_256,
            96 => &tables::window_tables::SINE_WINDOW_192,
            64 => &tables::window_tables::SINE_WINDOW_128,
            48 => &tables::window_tables::SINE_WINDOW_96,
            _ => return (0, 0),
        };

        debug_assert!(fac_w_zir_opt.is_some());
        let fac_w_zir = fac_w_zir_opt.unwrap();
        let mut read_input_from_ovl = false;

        // 1) Purge buffered synth_buf. Reads/writes overlap buffer samples.
        if ovl_offset != 0 && output_len > n_samples_written_out {
            data_out[data_out_offset..data_out_offset + ovl_offset]
                .copy_from_slice(&mdct.overlap[..ovl_offset]);
            n_samples_written_out = ovl_offset;
            ovl_offset = 0;
        } else if ovl_offset != 0 {
            read_input_from_ovl = true;
        }

        // Determine if overlap buffer is needed
        let divert_out0_to_ov = n_samples_written_out >= output_len;

        // 2) Handle old ACELP/ZIR/FAC data. Reads/writes fac_length samples.
        if fac_len != 0 {
            // Get tables
            // Set window tables

            // ACELP contribution in concealment case:
            // Use ZIR with a modified ZIR window to preserve some more energy.
            // Don't use FAC, which contains wrong information for concealed frame.
            // Don't use last ACELP samples, but double ZIR instead (afterwards).
            let do_fac_zir_conceal = last_frame_lost && (k == 0);
            if do_fac_zir_conceal {
                fac_w_zir[..fac_len].iter_mut().for_each(|x| *x = 0.0);
            };

            debug_assert!(&[48, 64, 96, 128].contains(&fac_len));
            let (fac_window_zir_table, fac_window_synth_table): (&[f32], &[f32]) = match fac_len {
                // coreCoderFrameLength = 1024
                128 => (&FAC_WINDOW_ZIR_128, &FAC_WINDOW_SYNTH_128),
                64 => (&FAC_WINDOW_ZIR_64, &FAC_WINDOW_SYNTH_64),
                // coreCoderFrameLength = 768
                96 => (&FAC_WINDOW_ZIR_96, &FAC_WINDOW_SYNTH_96),
                48 => (&FAC_WINDOW_ZIR_48, &FAC_WINDOW_SYNTH_48),
                _ => return (0, 0),
            };

            // Get buffer slices

            // Get FAC buffer.
            // Local lfac buffer, needed for frame loss case.
            let mut fac_buf_local: ArrayVec<f32, LFAC> = ArrayVec::new();
            let fac: &mut [f32];
            {
                let mut fac_buf_opt = FacData::get_memory_mut(
                    data_in,
                    &mut self.data_0,
                    self.data_index[k],
                    is_short,
                );

                if last_frame_lost || fac_buf_opt.is_none() {
                    fac_w_zir[..fac_len].iter_mut().for_each(|m| *m = 0.0);
                    for _ in 0..fac_len {
                        fac_buf_local.push(0.0);
                    }
                    fac = fac_buf_local.as_mut_slice();
                } else {
                    fac = fac_buf_opt.take().unwrap();
                }
            }
            debug_assert!(fac_len <= MAX_FRAMESIZE / (4 * 2));
            debug_assert!(fac.len() == fac_len);

            // Get input/output buffer slices.
            let input_this;
            let output_this;
            if divert_out0_to_ov {
                if read_input_from_ovl {
                    let (input_this_x, output_this_x) = mdct.overlap.split_at_mut(ovl_offset);
                    input_this = &(*input_this_x);
                    output_this = &mut output_this_x[..fac_len];
                } else {
                    input_this = &data_out[data_out_offset - fac_len..data_out_offset];
                    output_this = &mut mdct.overlap[ovl_offset..ovl_offset + fac_len];
                }
            } else {
                debug_assert!(!read_input_from_ovl);
                debug_assert!(
                    data_out_offset + n_samples_written_out >= fac_len || do_fac_zir_conceal
                );
                let output_this_x;
                (input_this, output_this_x) =
                    data_out.split_at_mut(data_out_offset + n_samples_written_out);
                output_this = &mut output_this_x[..fac_len];
            }

            {
                let do_deemph = last_lpd_mode != LpdMode::Tcx80;

                // Get FAC and FAC_ZIR buffer slice.
                if !do_fac_zir_conceal {
                    Self::calc_fac_signal(fac_w_zir, fac, a, is_fd_fac, true);
                }

                // Get windowed past ACELP samples and ACELP ZIR signal.
                {
                    // Get ACELP ZIR (pFac[]) and ACELP past samples (pOut0[]) and add them
                    // to the FAC synth signal contribution on pOut1[].
                    acelp_data.zir(a, fac, do_deemph);

                    // Get FAC and FAC_ZIR buffer slice.
                    if !do_fac_zir_conceal {
                        for (dst, src, fac, fac_w_zir, win_zir, win_syn) in izip!(
                            output_this.iter_mut(),
                            input_this.iter().rev(),
                            fac.iter(),
                            fac_w_zir.iter(),
                            fac_window_zir_table.iter(),
                            fac_window_synth_table.iter()
                        )
                        .take(fac_len)
                        {
                            *dst = *fac * *win_zir + *src * *win_syn + *fac_w_zir;
                        }
                    } else {
                        for (dst, fac, win_syn) in izip!(
                            output_this.iter_mut(),
                            fac.iter(),
                            fac_window_synth_table.iter()
                        )
                        .take(fac_len)
                        {
                            *dst = *fac * *win_syn * 2.0;
                        }
                    }
                    // Set offset to write FAC ZIR data later.
                    n_samples_read_fac += fac_len;
                }
            }
        }

        // 3a) IMDCT overlap add preparation. Processes TL samples
        if tl != 0 {
            let input_this = &mut data_in[k * (fac_len * 2)..k * (fac_len * 2) + tl];

            dct::dctiv(input_this);

            // Optional scaling of time domain - not yet windowed - of current spectrum.
            let mut len_inv = (tl as f32).recip();
            if gain != 0.0 {
                len_inv *= gain;
            }
            for iter in input_this.iter_mut().take(tl) {
                *iter *= len_inv;
            }
        }

        // 3b) IMDCT overlap add. Reads/writes FL/2 aka fac_len samples.
        if fac_len != 0 {
            // output samples before window crossing point NR .. TL/2.
            // -overlap[TL/2-NR..TL/2-NR-FL/2] + current[NR..TL/2]
            // output samples after window crossing point TL/2 .. TL/2+FL/2.
            // -overlap[0..FL/2] - current[TL/2..FL/2]
            // Setup window slices
            let input_this_x = &mut data_in[k * (fac_len * 2)..k * (fac_len * 2) + tl];
            // Spectrum input buffer used in this scope.
            let input_this = &input_this_x[tl - fac_len..];
            // Buffer for output of this scope.
            let output_that: &mut [f32];

            if divert_out0_to_ov {
                // Divert output_that to overlap buffer.
                output_that = &mut mdct.overlap[ovl_offset..ovl_offset + fac_len];
                ovl_offset += fac_len;
            } else {
                // oOverlap buffer not needed --> use output buffer.
                output_that = &mut data_out[data_out_offset + n_samples_written_out
                    ..data_out_offset + n_samples_written_out + fac_len];
                n_samples_written_out += fac_len;
            }

            for (dst, src, win) in izip!(
                output_that.iter_mut().rev(),
                input_this.iter(),
                window_table.iter()
            ) {
                *dst -= *src * win.re;
            }
        }

        // 4) FAC_ZIR part. Reads/writes NL samples.
        if nl != 0 {
            debug_assert!(nl >= fac_len);
            // Setup window slices
            let input_this_x = &mut data_in[k * (fac_len * 2)..k * (fac_len * 2) + tl];
            let (input_this, input_that) =
                input_this_x[tl - fac_len - nl..tl - fac_len].split_at(nl - fac_len);
            let (output_this, output_that);
            if divert_out0_to_ov {
                // divert output_that to overlap buffer
                (output_this, output_that) =
                    mdct.overlap[ovl_offset..ovl_offset + nl].split_at_mut(fac_len);
                ovl_offset += nl;
            } else {
                // overlap buffer not needed --> use output buffer
                (output_this, output_that) = data_out[data_out_offset + n_samples_written_out
                    ..data_out_offset + n_samples_written_out + nl]
                    .split_at_mut(fac_len);
                n_samples_written_out += nl;
            }

            debug_assert!(fac_len <= nl); // Make sure FAC buffer will be drained.
            for (dst, src, fac) in izip!(
                output_this.iter_mut(),
                input_that.iter().rev(),
                fac_w_zir[fac_len..].iter()
            )
            .take(fac_len.min(nl))
            {
                // Synthesis filter Zir component, FAC ZIR again
                *dst = -*src + *fac;
            }

            for (dst, src) in
                izip!(output_that.iter_mut(), input_this.iter().rev()).take(nl - fac_len)
            {
                *dst = -*src;
            }

            // FAC ZIR has now been added.
            n_samples_read_fac += fac_len;
        }

        // 5) Loop over short windows, for ACELP -> FD short
        if is_short {
            debug_assert!(is_fd_fac);

            fl = fr;
            nl = nr;
            let tl_inv = (tl as f32).recip();

            for spec_current in data_in.chunks_exact_mut(tl).skip(1) {
                dctiv(spec_current);
            }

            for iter in data_in[tl..].iter_mut() {
                *iter *= tl_inv;
            }

            for i in 0..MAX_WINDOWS - 1 {
                // Determine if overlap buffer is needed.
                let divert_out0_to_ov = n_samples_written_out + nr + fl / 2 > output_len;
                let divert_out1_to_ov = n_samples_written_out + nr + fl + nl > output_len;
                let spectrum_last = &data_in[i * tl..(i + 1) * tl];
                let spectrum_curr = &data_in[(i + 1) * tl..(i + 2) * tl];

                // Set buffer references for this window.
                let ovl_len = mdct.overlap.len();
                // Overlap buffer of this windows's spectrum.
                let overlap_current =
                    &mut mdct.overlap[ovl_offset..(ovl_offset + nr + fl + nl).min(ovl_len)];
                // Output buffer of this windows's spectrum.
                let output_current = &mut data_out[data_out_offset + n_samples_written_out
                    ..(data_out_offset + n_samples_written_out + nr + fl + nl).min(output_len)];

                // Process NR samples.
                {
                    // Setup window slices.
                    // Input of overlap buffer used in this scope.
                    let input_that = &spectrum_last[spectrum_last.len() - nr..];
                    let output_this;
                    if !divert_out0_to_ov {
                        // Account output samples.
                        output_this = &mut output_current[..nr];
                        n_samples_written_out += nr;
                    } else {
                        // Divert output first half to overlap buffer if we already
                        // got enough output samples.
                        output_this = &mut overlap_current[..nr];
                        ovl_offset += nr;
                    }

                    // NR synth_buf samples 0 .. NR. -overlap[TL/2..TL/2-NR]
                    for (dst, src) in
                        izip!(output_this.iter_mut(), input_that.iter().rev()).take(nr)
                    {
                        *dst = *src;
                    }
                }

                // Process FL samples
                {
                    // Setup window slices
                    // Input of overlap buffer used in this scope
                    let input_this = &spectrum_curr[tl - fl / 2..tl];
                    // Input of overlap buffer used in this scope
                    let input_that = &spectrum_last[..tl / 2 - nr];
                    // Buffer for second part of output of this scope
                    let (output_this, output_that);
                    if divert_out0_to_ov {
                        // Use overlap buffer for all output
                        (output_this, output_that) =
                            overlap_current[nr..nr + fl].split_at_mut(fl / 2);
                        ovl_offset += fl;
                    } else if divert_out1_to_ov {
                        // Divert only output_that to overlap buffer
                        ovl_offset += fl / 2;
                        n_samples_written_out += fl / 2;
                        (output_this, output_that) = (
                            &mut output_current[nr..nr + fl / 2],
                            &mut overlap_current[..fl / 2],
                        )
                    } else {
                        // Overlap buffer not needed --> only use output buffer
                        (output_this, output_that) =
                            output_current[nr..nr + fl].split_at_mut(fl / 2);
                        n_samples_written_out += fl;
                    }

                    // Output samples before window crossing point NR .. TL/2.
                    // -overlap[TL/2-NR..TL/2-NR-FL/2] + current[NR..TL/2]
                    // Output samples after window crossing point TL/2 .. TL/2+FL/2.
                    // -overlap[0..FL/2] - current[TL/2..FL/2]
                    for (out_re, out_im, in_re, in_im, window) in izip!(
                        output_that.iter_mut().rev(),
                        output_this.iter_mut(),
                        input_this.iter(),
                        input_that.iter().rev(),
                        wrs.iter(),
                    ) {
                        let x = Complex {
                            re: *in_re,
                            im: -(*in_im),
                        };
                        let y = x * window;

                        *out_im = y.im;
                        *out_re = -y.re;
                    }
                }

                // Process NL samples
                {
                    // Setup window slices
                    // Spectrum input buffer of this scope
                    let input_this = &spectrum_curr[tl - fl / 2 - nl..tl - fl / 2];
                    let output_this;
                    if divert_out0_to_ov {
                        // Use overlap buffer
                        output_this = &mut overlap_current[nr + fl..nr + fl + nl];
                        ovl_offset += nl;
                    } else if divert_out1_to_ov {
                        // Use overlap buffer
                        output_this = &mut overlap_current[fl / 2..fl / 2 + nl];
                        ovl_offset += nl;
                    } else {
                        // Overlap buffer not needed --> only use output buffer
                        output_this = &mut output_current[nr + fl..nr + fl + nl];
                        n_samples_written_out += nl;
                    };

                    /* NL output samples TL/2+FL/2..TL. - current[FL/2..0] */
                    for (dst, src) in izip!(output_this.iter_mut(), input_this.iter().rev()) {
                        *dst = -*src;
                    }
                }

                // Add FAC ZIR of the ACELP -> MDCT transition of block 0 to block 1
                // This condition must only be true in the first iteration over subframes,
                // because `fac_w_zir_opt` is set to None explicitely here.
                if n_samples_read_fac < 2 * fac_len {
                    // Setup window slices
                    let output_this = if divert_out0_to_ov {
                        // Use overlap buffer for all output
                        &mut overlap_current[nr..nr + fl / 2]
                    } else {
                        // Overlap buffer not needed --> only use output buffer
                        &mut output_current[nr..nr + fl / 2]
                    };
                    let fac_zir_option = &mut fac_w_zir.get(fac_len..2 * fac_len);
                    let bytes_copied = Self::drain_fac_zir(output_this, fac_zir_option);
                    debug_assert!(bytes_copied == fl / 2);
                    n_samples_read_fac += bytes_copied;
                }
            }
            // Save overlap.
            {
                debug_assert!(ovl_offset + tl / 2 <= mdct.overlap.len());
                let spectrum_current = &data_in[(MAX_WINDOWS - 1) * tl..MAX_WINDOWS * tl];
                let input_this = &spectrum_current[..tl / 2];
                let output_this = &mut mdct.overlap[ovl_len - tl / 2..];

                for (dst, src) in izip!(output_this, input_this) {
                    *dst = *src;
                }
            }
        } else {
            // No short windows.
            // Save overlap.
            {
                debug_assert!(ovl_offset + tl / 2 <= mdct.overlap.len());
                let spectrum_current = if is_fd_fac {
                    &data_in[..tl]
                } else {
                    &data_in[k * (fac_len * 2)..k * (fac_len * 2) + tl]
                };
                let input_this = &spectrum_current[..tl / 2];
                let output_this = &mut mdct.overlap[ovl_len - tl / 2..];

                for (dst, src) in izip!(output_this, input_this) {
                    *dst = *src;
                }
            }
        }

        // Set MDCT's previous window values.
        mdct.set_ov_offset(ovl_offset);
        mdct.set_prev_window(Some(wrs), nr, tl);

        (n_samples_written_out, n_samples_read_fac)
    }

    /// Applies TCX and ALFD gains to FAC data.
    ///
    /// # Parameters
    ///
    /// - `data`: FAC data.
    /// - `tcx_gain`: TCX gain.
    /// - `alfd_gains`: ALFD gains.
    /// - `mode`: LPD mode of TCX frame where the FAC signal needs to be applied.
    /// - `k`: Index of current subframe.
    ///
    /// # Examples
    ///
    /// Appyly ALFD gains to FAC data.
    ///
    /// ```
    /// use aac::aac_dec::lpd::common::LpdMode;
    /// use aac::aac_dec::lpd::constants::*;
    /// use aac::aac_dec::lpd::fac::FacData;
    /// use aac::aac_dec::*;
    /// use aac::common::bitstream::{Bitstream, Mode};
    ///
    /// let buffer = vec![0; 8];
    /// let mut bs = Bitstream::new(buffer.len(), Mode::Reader);
    /// bs.init(&buffer, 40);
    /// let mut data = [0.0f32; NB_DIV * L_DIV];
    ///
    /// let mut fac = FacData::new();
    /// let mut modes = [
    ///     LpdMode::Tcx20,
    ///     LpdMode::Tcx20,
    ///     LpdMode::Tcx20,
    ///     LpdMode::Tcx20,
    /// ];
    ///
    /// let alfd_gains = [0.5; 32];
    /// let tcx_gain = 8.0;
    ///
    /// let k = 0;
    ///
    /// let is_short = false;
    /// let use_gain = false;
    ///
    /// fac.read(&mut bs, &mut data, is_short, Some(&modes), use_gain, k);
    /// fac.apply_gains(&mut data, tcx_gain, &alfd_gains, modes[k], k);
    /// ```
    pub fn apply_gains(
        &mut self,
        data: &mut [f32],
        tcx_gain: f32,
        alfd_gains: &[f32],
        mode: LpdMode,
        k: usize,
    ) {
        // Table is also correct for coreCoderFrameLength = 768.
        // Factor 3/4 is canceled out: gainFac = 0.5 * sqrt(fac_length/lFrame)

        //                                TCX20              TCX40 TCX80
        static GAIN_FAC: [f32; 4] = [0.5, 0.353553390596061, 0.25, 0.176776695065200];

        debug_assert!(mode.is_tcx());

        let fac_data =
            FacData::get_memory_mut(data, &mut self.data_0, self.data_index[k], false).unwrap();
        let fac_data_len = fac_data.len();
        assert!((fac_data_len == 128) || (fac_data_len == 96));

        let fac_factor = GAIN_FAC[usize::from(mode)] * tcx_gain;

        // Apply spectrum deshaping using alfd_gains.
        // Directly also apply gain factor to FAC data.
        fac_data[..fac_data_len >> 2]
            .iter_mut()
            .enumerate()
            .for_each(|(i, x)| {
                *x *= alfd_gains[i >> (3 - u8::from(mode))] * fac_factor;
            });

        // Only apply gain factor to rest of FAC data.
        fac_data[fac_data_len >> 2..].iter_mut().for_each(|x| {
            *x *= fac_factor;
        });
    }

    /// Does FAC transition from frequency to ACELP domain.
    ///
    /// # Parameters
    ///
    /// - `data_in`: MDCT spectrum input.
    /// - `data_out`: Time domain output.
    /// - `fac_w_zir_opt`: Optional FAC buffer, incl. FAC ZIR.
    /// - `a`: LP domain filter coefficients.
    /// - `mdct`: MDCT instance.
    /// - `prev_window_shape`: Previous window shape.
    /// - `k`: Index of current subframe.
    /// - `is_short`: Was MDCT frame short blocks?
    /// - `last_frame_lost`: Was last frame lost?
    /// - `is_fd_fac`: Indicates FAC processing from FD.
    ///
    /// # Return
    /// - (`written samples`, `read_samples_fac`)
    #[expect(clippy::too_many_arguments)]
    pub fn mdct2acelp(
        &mut self,
        data_in: &mut [f32],
        data_out: &mut [f32],
        fac_w_zir_opt: &Option<&[f32]>,
        a: &[f32; M_LP_FILTER_ORDER],
        mdct: &mut mdct::Mdct,
        prev_window_shape: WindowShape,
        k: usize,
        is_short: bool,
        last_frame_lost: bool,
        is_fd_fac: bool,
    ) -> (usize, usize) {
        let mut n_samples_written_out = 0;
        let output_len = data_out.len();

        // Derive FL.
        let fac_len = if let Some(fac) =
            FacData::get_memory_mut(data_in, &mut self.data_0, self.data_index[k], is_short)
        {
            fac.len()
        } else {
            FacData::fac_len(output_len, is_short)
        };
        let mut fl = fac_len * 2;

        // Get MDCT data.
        let mut ovl_offset = mdct.ov_offset;
        let ovl_len = mdct.overlap.len();
        let (prev_fr, mut prev_nr) = if let Some(prev_window) = mdct.prev_wrs {
            (prev_window.len() * 2, mdct.prev_nr)
        } else {
            (0, 0)
        };
        let prev_window = tables::window_tables::get_table(fl as u16, prev_window_shape).unwrap();

        // Adapt window slope length in case of frame loss.
        if prev_fr != fl {
            let mut nl = 0;
            mdct.adapt_parameters(&mut nl, prev_window, &mut fl, data_out.len());
            prev_nr = if mdct.prev_wrs.is_some() {
                mdct.prev_nr
            } else {
                0
            };
            debug_assert!(nl == 0);
        }

        // 1) Get overlap time data.
        if ovl_offset != 0 && ovl_offset <= output_len {
            let ovl_buf = &mut mdct.overlap[..];
            data_out[..ovl_offset].copy_from_slice(&ovl_buf[..ovl_offset]);
            n_samples_written_out += ovl_offset;
            ovl_offset = 0;
        }

        // Determine if overlap buffer is needed for writing output.
        let divert_out_to_ov = n_samples_written_out >= output_len;

        // 2) Write previous NR part.
        if prev_nr != 0 {
            // Setup window slices
            let input_this: &mut [f32]; // input buffer used in this scope
            let output_this: &mut [f32]; // output buffer used in this scope

            if divert_out_to_ov {
                // Use overlap buffer for all output
                let (output_this_x, input_this_x) = mdct.overlap[..].split_at_mut(ovl_offset);
                let input_this_x_len = input_this_x.len();
                input_this = &mut input_this_x[input_this_x_len - prev_nr..];
                output_this = &mut output_this_x[..prev_nr];
                ovl_offset += prev_nr;
            } else {
                // Overlap buffer not needed --> only use output buffer
                input_this = &mut (mdct.overlap)[ovl_len - prev_nr..ovl_len];
                output_this = &mut data_out[n_samples_written_out..n_samples_written_out + prev_nr];
                n_samples_written_out += prev_nr;
            }

            for (dst, src) in izip!(output_this.iter_mut(), input_this.iter().rev()) {
                *dst = -*src
            }
        }

        // 3) Write FL samples (FAC and FAC_ZIR)
        let mut n_samples_read_fac = 0;
        if fac_len != 0 {
            // Setup window slices
            let input_this: &mut [f32]; // input buffer used in this scope
            let output_this: &mut [f32]; // output buffer used in this scope

            if divert_out_to_ov {
                // Use overlap buffer for all output
                let slice_len = mdct.overlap.len() - ovl_offset;
                let (output_this_x, input_this_x) =
                    mdct.overlap[ovl_offset..].split_at_mut(slice_len - prev_nr - fac_len);
                input_this = &mut input_this_x[..fac_len];
                output_this = &mut output_this_x[..fac_len];
                ovl_offset += fac_len;
            } else {
                // Overlap buffer not needed for output --> use output buffer
                input_this = &mut (mdct.overlap)[ovl_len - prev_nr - fac_len..ovl_len - prev_nr];
                output_this = &mut data_out[n_samples_written_out..n_samples_written_out + fac_len];
                n_samples_written_out += fac_len;
            }

            // Get fac_zir buffer
            let mut fac_opt: Option<&mut [f32]> = None;
            if !last_frame_lost {
                fac_opt = FacData::get_memory_mut(
                    data_in,
                    &mut self.data_0,
                    self.data_index[k],
                    is_short,
                );
            }

            // Write FAC w/ ZIR buffer
            if let Some(fac) = fac_opt {
                // Note: The FAC gain might have been applied directly after bit stream
                // parse in this case.
                debug_assert!(fac.len() <= MAX_FRAMESIZE / (4 * 2));
                // We only have output_this.len()==fac_len samples, which is less than 2*fac_len.
                // Therefore, we copy to a temporary buffer and then store only fac_len
                // samples in output_this.
                FacData::calc_fac_signal(&mut output_this[..fac_len], fac, a, is_fd_fac, false);
            } else {
                // Clear buffer because of the overlap and ADD.
                output_this[..fac_len].iter_mut().for_each(|m| *m = 0.0);
            }

            // Write Overlap Add
            for (dst, src, win) in izip!(
                output_this[..fac_len].iter_mut(),
                input_this.iter().rev(),
                prev_window.iter()
            ) {
                *dst -= *src * win.re
            }

            // Write FAC and FAC_ZIR (ACELP -> TCX20 -> ACELP transition case)
            if fac_w_zir_opt.is_some() {
                let bytes_copied = Self::drain_fac_zir(&mut output_this[..fac_len], fac_w_zir_opt);
                debug_assert!(bytes_copied == fl / 2);
                n_samples_read_fac += bytes_copied;
            }
        }

        // Store back MDCT data
        mdct.set_prev_window(None, 0, 0);
        mdct.set_ov_offset(ovl_offset);

        (n_samples_written_out, n_samples_read_fac)
    }

    /// Creates a properly initialized FAC instance.
    pub fn new() -> Self {
        Default::default()
    }

    /// Reads FAC data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream to read FAC data from.
    /// - `data`: Spectral data read from bitstream.
    /// - `is_short`: Indicates short frames.
    /// - `mode`: Current LPD mode.
    /// - `use_gain`: Use gain?
    /// - `k`: Index of current subframe.
    ///
    /// # Examples
    ///
    /// Read FAC parameters from bitstream.
    ///
    /// ```
    /// use aac::aac_dec::lpd::common::LpdMode;
    /// use aac::aac_dec::lpd::constants::*;
    /// use aac::aac_dec::lpd::fac::FacData;
    /// use aac::aac_dec::*;
    /// use aac::common::bitstream::{Bitstream, Mode};
    ///
    /// let buffer = vec![0; 8];
    /// let mut bs = Bitstream::new(buffer.len(), Mode::Reader);
    /// bs.init(&buffer, 40);
    /// let mut data = [0.0f32; NB_DIV * L_DIV];
    ///
    /// let mut fac = FacData::new();
    /// let mut modes = [
    ///     LpdMode::Acelp,
    ///     LpdMode::Acelp,
    ///     LpdMode::Acelp,
    ///     LpdMode::Acelp,
    /// ];
    ///
    /// let k = 0;
    ///
    /// let is_short = false;
    /// let use_gain = false;
    ///
    /// fac.read(&mut bs, &mut data, is_short, Some(&modes), use_gain, k);
    /// ```
    pub fn read(
        &mut self,
        bs: &mut Bitstream,
        data: &mut [f32],
        is_short: bool,
        mode: Option<&[LpdMode; NB_DIV]>,
        use_gain: bool,
        k: usize,
    ) -> i32 {
        let mut fac_gain = 0.0f32;
        let mut a_fac = [0f32; LFAC];

        self.assign_memory(mode, k);
        if let Some(fac_data) =
            FacData::get_memory_mut(data, &mut self.data_0, self.data_index[k], is_short)
        {
            let fac_data_len = fac_data.len();

            if use_gain {
                fac_gain = lpd_common::decode_gain(bs.read(7) as u8);
            }

            if lpc::decode_avq(bs, &mut a_fac, 1, 1, fac_data.len()) != 0 {
                return -1;
            }

            if use_gain {
                izip!(fac_data.iter_mut(), a_fac[..fac_data_len].iter())
                    .for_each(|(dst, src)| *dst = *src * fac_gain);
            } else {
                fac_data.copy_from_slice(&a_fac[..fac_data_len]);
            }

            0
        } else {
            // Could not get FAC memory.
            1
        }
    }

    /// Resets the FAC module.
    pub fn reset(&mut self) {
        self.data_index.fill(-1);
        self.mem_state = 0;
        self.data_0.fill(0.0);
    }

    /// Assigns memory to store FAC data.
    ///
    /// # Parameters
    ///
    /// - `mode_opt`: Optional LPD mode array for every subframe.
    /// - `k`: Index of current subframe.
    fn assign_memory(&mut self, mode_opt: Option<&[LpdMode; NB_DIV]>, k: usize) {
        let mut index = MAX_WINDOWS;
        if let Some(mode) = mode_opt {
            assert!(self.mem_state < MAX_WINDOWS as u8);

            // Look for free space to store FAC data. 2 FAC data blocks fit into each ACELP
            // data block.
            for i in self.mem_state as usize..MAX_WINDOWS {
                if mode[i >> 1] == LpdMode::Acelp {
                    index = i;
                    break;
                }
            }

            self.mem_state = (index + 1) as u8;
        }

        self.data_index[k] = index as i8;
    }

    /// Calculates FAC and FAC ZIR.
    ///
    /// # Parameters
    ///
    /// - `fac_w_zir`: FAC and FAC ZIR signal. Must be twice the size of FAC input.
    /// - `fac_only`: FAC input data.
    /// - `a`: LP coefficients.
    /// - `is_fd_fac`: Indicates that FD is used.
    fn calc_fac_signal(
        fac_w_zir: &mut [f32],
        fac_only: &mut [f32],
        a: &[f32; 16],
        is_fd_fac: bool,
        do_write_zir: bool,
    ) {
        let fac_len = fac_only.len();
        debug_assert!(fac_w_zir.len() == if do_write_zir { 2 * fac_len } else { fac_len });
        // Compute inverse DCT-IV of FAC data.
        dctiv(fac_only);

        let (out_fac, out_zir) = fac_w_zir.split_at_mut(fac_len);

        if !is_fd_fac {
            // Scale the FAC when frame is TCX
            let inv_fac = (fac_len as f32).recip();
            izip!(out_fac, fac_only).for_each(|(dst, src)| *dst = *src * inv_fac);
        } else {
            izip!(out_fac, fac_only).for_each(|(dst, src)| *dst = *src);
        }

        let mut w_a: [f32; M_LP_FILTER_ORDER] = [0.0f32; M_LP_FILTER_ORDER];
        lpc::apply_abs_weighting(&mut w_a, a);

        if do_write_zir {
            // We need the output of the IIR filter to be longer than "fac_len".
            // For this reason we run it with zero input appended to the end of the input
            // sequence, i.e. we generate its ZIR and extend the output signal.
            out_zir[..fac_len].iter_mut().for_each(|x| *x = 0.0); // memset to 0

            // Apply weighted synthesis filter to FAC data, including optional ZIR.
            Self::syn_filt_zero(&w_a, &mut fac_w_zir[..2 * fac_len]);
        } else {
            Self::syn_filt_zero(&w_a, &mut fac_w_zir[..fac_len])
        }
    }

    /// Adds values of FAC ZIR buffer to destination buffer.
    fn drain_fac_zir(dst_buf: &mut [f32], fac_zir_buf_opt: &Option<&[f32]>) -> usize {
        if let Some(fac_zir_buf) = *fac_zir_buf_opt {
            debug_assert!(fac_zir_buf.len() <= dst_buf.len());
            for (dst, src) in izip!(dst_buf, fac_zir_buf) {
                *dst += *src;
            }
            fac_zir_buf.len()
        } else {
            0
        }
    }

    /// Gets length of FAC data, depending on short/long blocks.
    pub fn fac_len(frame_len: usize, is_short: bool) -> usize {
        if is_short {
            frame_len / MAX_WINDOWS / 2
        } else {
            frame_len / MAX_WINDOWS
        }
    }

    /// Gets memory where FAC data is stored.
    ///
    /// # Parameters
    ///
    /// - `data`: Memory for four subframes.
    /// - `data_extra`: Additional memory of the size of one subframe.
    /// - `index`: Subframe index where FAC info is stored.
    /// - `is_short`:  Indicates if the current frame is a short frame.
    fn get_memory_mut<'a>(
        data: &'a mut [f32],
        data_extra: &'a mut [f32],
        index: i8,
        is_short: bool,
    ) -> Option<&'a mut [f32]> {
        let slice_len = FacData::fac_len(data.len(), is_short);

        match (index as usize).cmp(&MAX_WINDOWS) {
            Ordering::Less => {
                let slice_pos = data.len() / MAX_WINDOWS * index as usize;
                Some(&mut data[slice_pos..slice_pos + slice_len])
            }
            Ordering::Equal => Some(&mut data_extra[0..slice_len]),
            Ordering::Greater => None,
        }
    }

    /// Applies synthesis filter with zero input to x. The overall filter gain is 1.0.
    ///
    /// # Parameters
    ///
    /// - `a`: LPC filter coefficients.
    /// - `x`: Input/output vector, where the synthesis filter is applied in place.
    fn syn_filt_zero(a: &[f32], x: &mut [f32]) {
        for i in 0..x.len() {
            let mut l_tmp = 0.0f32;
            for j in 0..M_LP_FILTER_ORDER.min(i) {
                l_tmp -= a[j] * x[i - (j + 1)];
            }
            x[i] += l_tmp.clamp(-LPD_SYN_FILT_LIMIT, LPD_SYN_FILT_LIMIT);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::common::tables::window_tables;

    #[test]
    fn fac_memory_rw() {
        //&mut self, mode_opt: Option<&[usize; NB_DIV]>, arr_idx: usize
        let mut fac_data: FacData = Default::default();
        let lpd_mode: [LpdMode; NB_DIV] = [
            LpdMode::Tcx20,
            LpdMode::Tcx20,
            LpdMode::Acelp,
            LpdMode::Tcx20,
        ];
        let mut spectrum = [0.0f32; L_FRAME_PLUS_1024];

        let spec2_acelp = &mut spectrum[2 * L_DIV_1024..3 * L_DIV_1024];

        spec2_acelp[0] = 123.4;
        spec2_acelp[L_DIV_1024 - 1] = 234.5;

        fac_data.reset();

        // Set data in extra memory
        fac_data.data_0[0] = 345.6;

        // Assign memory for frame index 0
        let mut frame_idx = 0;
        fac_data.assign_memory(Some(&lpd_mode), frame_idx);
        assert!(fac_data.mem_state == 5);
        assert!(fac_data.data_index[frame_idx] == 4);

        // Assign memory for frame index 1
        frame_idx += 1;
        fac_data.assign_memory(Some(&lpd_mode), frame_idx);
        assert!(fac_data.mem_state == 6);
        assert!(fac_data.data_index[frame_idx] == 5);

        // Assign memory for frame index 2 --> extra memory should be used!
        frame_idx += 1;
        fac_data.assign_memory(Some(&lpd_mode), frame_idx);
        assert!(fac_data.mem_state == 9);
        assert!(fac_data.data_index[frame_idx] == 8);

        // Get memory for frame index 0
        frame_idx = 0;
        let fac_mem = FacData::get_memory_mut(
            &mut spectrum,
            &mut fac_data.data_0,
            fac_data.data_index[frame_idx],
            true,
        );
        assert!(fac_mem.unwrap()[0] == 123.4);

        // Get memory for frame index 1
        frame_idx += 1;
        let fac_mem = FacData::get_memory_mut(
            &mut spectrum,
            &mut fac_data.data_0,
            fac_data.data_index[frame_idx],
            false,
        );
        assert!(fac_mem.unwrap()[L_DIV_1024 / 2 - 1] == 234.5);

        // Get memory for frame index 2
        frame_idx += 1;
        let fac_mem = FacData::get_memory_mut(
            &mut spectrum,
            &mut fac_data.data_0,
            fac_data.data_index[frame_idx],
            false,
        );
        assert!(fac_mem.unwrap()[0] == 345.6);
    }

    #[test]
    #[should_panic(expected = "assertion failed: self.mem_state < MAX_WINDOWS as u8")]
    fn fac_memory_too_much() {
        //&mut self, mode_opt: Option<&[usize; NB_DIV]>, arr_idx: usize
        let mut fac_data: FacData = Default::default();
        let lpd_mode: [LpdMode; NB_DIV] = [
            LpdMode::Tcx20,
            LpdMode::Tcx20,
            LpdMode::Acelp,
            LpdMode::Tcx20,
        ];

        fac_data.reset();

        // Assign memory for frame index 0
        let mut frame_idx = 0;
        fac_data.assign_memory(Some(&lpd_mode), frame_idx);

        // Assign memory for frame index 1
        frame_idx += 1;
        fac_data.assign_memory(Some(&lpd_mode), frame_idx);

        // Assign memory for frame index 2 --> extra memory should be used!
        frame_idx += 1;
        fac_data.assign_memory(Some(&lpd_mode), frame_idx);
        assert!(fac_data.mem_state == 9);
        assert!(fac_data.data_index[frame_idx] == 8);

        // Try to assign more memory - this should panic, since no memory is available any more
        frame_idx += 1;
        fac_data.assign_memory(Some(&lpd_mode), frame_idx);
    }

    #[test]
    fn syn_filt_zero() {
        let lpc_coeffs = [0.0f32; M_LP_FILTER_ORDER];
        let mut fac_buf = [0.0f32; 2 * LFAC_1024];

        let mut fac_data: FacData = Default::default();
        fac_data.reset();

        FacData::syn_filt_zero(&lpc_coeffs, &mut fac_buf);

        assert!(fac_buf == [0.0f32; 2 * LFAC_1024]);
    }

    #[test]
    fn calc_fac_signal() {
        let mut fac_in = [0.0f32; LFAC_1024];
        let mut fac_out = [0.0f32; 2 * LFAC_1024];
        let lpc_coeffs = [0.0f32; M_LP_FILTER_ORDER];
        let is_fd_fac = false;

        FacData::calc_fac_signal(&mut fac_out, &mut fac_in, &lpc_coeffs, is_fd_fac, true);

        assert!(fac_out == [0.0f32; 2 * LFAC_1024]);
    }

    #[test]
    /// Tests applying gains on the FAC data of a given frame.
    ///
    /// By applying alfd_gains*tcx_gain*0.25 on the first quarter
    /// and tcx_gain*0.25 on the remaining three quarters of half of the
    // FAC frame, the first half of the FAC frame will be 1.0.
    ///
    /// # Whole frame
    ///
    ///  FAC_DATA TCX20    TCX40
    /// +--------+--------+----------------+
    /// |fac data|        |this frame      |
    /// +--------+--------+----------------+
    ///
    /// # FAC data before applying gains
    /// +--------------------------------+
    /// |1s.|0.5s.......|0s..............|
    /// +--------------------------------+
    /// First quarter 1s, three quarters after that 0.5s
    ///
    /// # FAC data after applying gains
    /// +--------------------------------+
    /// |1s.............|0s..............|
    /// +--------------------------------+
    fn apply_gains() {
        let mut fac_data: FacData = Default::default();
        let mut spectrum = [0.0f32; L_FRAME_PLUS_1024];
        let alfd_gains = [0.5; 32]; // multiplied with fac_factor in the first quarter
        let tcx_gain = 8.0; // results in a fac_factor of 2 (b/o TCX40 --> * 0.25)
        let lpd_modes = [
            LpdMode::Acelp,
            LpdMode::Tcx20,
            LpdMode::Tcx40,
            LpdMode::Undef, // only three subframes, since TCX40 is twice as big as TCX20 or ACELP
        ];

        // Assign memory for frame index 0
        let frame_idx: usize = 2; /* last subframe, i.e. the TCX40 frame */
        fac_data.assign_memory(Some(&lpd_modes), 2);
        assert!(fac_data.mem_state == 1);

        // Store FAC test data
        let fac_idx = fac_data.data_index[frame_idx] as usize;
        // Fill first quarter with 1s.
        spectrum[L_DIV_1024 * fac_idx
            ..(L_DIV_1024 as f32 * fac_idx as f32 + LFAC_1024 as f32 * 0.25) as usize]
            .clone_from_slice(&[1.0f32; (LFAC_1024 / 4)]);
        // Fill remaining three quarters with 0.5s.
        spectrum[(L_DIV_1024 as f32 * fac_idx as f32 + LFAC_1024 as f32 * 0.25) as usize
            ..(L_DIV_1024 as f32 * fac_idx as f32) as usize + LFAC_1024]
            .clone_from_slice(&[0.5f32; (LFAC_1024 * 3 / 4)]);

        fac_data.apply_gains(
            &mut spectrum,
            tcx_gain,
            &alfd_gains,
            lpd_modes[frame_idx],
            frame_idx,
        );

        // The TCX40 FAC data , i.e. the first subframe, should, over the
        // length fac_len, be completetly filled with 1's now
        assert!(
            spectrum[L_DIV_1024 * fac_idx..L_DIV_1024 * fac_idx + LFAC_1024]
                == [1.0f32; L_DIV_1024 / 2]
        );
        // The rest of the spectrum must still be 0!
        assert!(spectrum[L_DIV_1024 * fac_idx + LFAC_1024..] == [0.0f32; L_DIV_1024 * 7 / 2]);
    }

    #[test]
    fn mdct2acelp_long_frames_no_fac() {
        let mut fac_data: FacData = Default::default();

        const FRAMESIZE: usize = 768;
        const LFAC_768: usize = 96;

        // Buffers
        #[rustfmt::skip]
        let mut data_in: [f32; 768] = [
            16628.5859, 13636.9561, 1515.21729, -25758.6934, 17801.9805, -11756.9834, -11756.9834, 2351.39648, 0.00000000, -5571.34180, 0.00000000, 11142.6836, -11142.6836, 0.00000000, -5571.34180, 11142.6836, -5571.34180, -5571.34180, 5571.34180, 0.00000000,
            5571.34180, 0.00000000, -5571.34180, 5571.34180, 0.00000000, -11142.6836, 5571.34180, 0.00000000, 0.00000000, 11142.6836, -11142.6836, 5571.34180, 0.00000000, 0.00000000, 0.00000000, -5571.34180, 5571.34180, 0.00000000, 0.00000000, 0.00000000,
            -8357.01270, 2785.67090, -2785.67090, 2785.67090, 2785.67090, -2785.67090, 2785.67090, 2785.67090, 0.00000000, -5571.34180, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, -5571.34180, 5571.34180, 5571.34180, 0.00000000, 0.00000000,
            0.00000000, 0.00000000, 0.00000000, 0.00000000, -5571.34180, 5571.34180, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
            0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
            0.00000000, 0.00000000, 3947.54565, -3947.54565, 3947.54565, -3947.54565, 3947.54565, -3947.54565, -3947.54565, 3947.54565, 3947.54565, 3947.54565, 3947.54565, -3947.54565, -3947.54565, -3947.54565, -3947.54565, 3947.54565, 0.00000000, 0.00000000,
            0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
            0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 3947.54565, -3947.54565, -3947.54565, -3947.54565, -3947.54565, 3947.54565, 3947.54565, 3947.54565, 0.00000000, 0.00000000, 0.00000000,
            0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
            0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, -1303.82715, -1283.84387, -902.437744, -569.331604, -155.738907, 229.943863, 270.602570, -153.938980, -358.804840, -340.129822, -493.189026, -134.018738,
            54.3472748, 209.533997, 558.227783, 946.161682, 1006.85553, 926.835144, 614.287048, 431.745514, 351.467255, 243.072495, 306.459442, 385.298798, 494.323792, 695.139282, 855.356812, 1082.20740, 1251.83691, 1368.32312, 1309.72681, 935.405884, 599.451538,
            35.6674271, -446.340515, -849.126770, -1127.67065, -1096.17041, -720.777405, -393.334412, -100.449348, 149.411224, 52.6758614, -285.212616, -690.674500, -1049.91748, -920.106873, -789.934692, -586.156860, -40.4311676, 435.512054, 966.683655, 1280.13513,
            1196.11572, 963.974792, 679.680420, 368.705902, -49.4520569, -413.084442, -475.896637, -360.318329, -21.2329082, 45.9487000, 252.659576, 756.111206, 1107.50952, 1310.00110, 1373.33569, 1038.02332, 626.884644, 429.703369, 77.9613342, -237.231796,
            -239.379211, -165.359451, -365.100250, -476.436401, -653.155273, -934.191101, -1414.84326, -1714.17322, -1853.98022, -1678.57739, -1117.33740, -500.800812, 184.191727, 770.837463, 1284.04285, 1525.97534, 1384.99097, 1058.63892, 519.526123, 54.4562111,
            -372.841248, -974.807190, -1113.51709, -1058.34265, -854.338196, -680.580688, -459.454681, -133.861084, 214.333771, 563.529114, 561.533081, 469.786835, 332.447815, 154.138550, -127.256760, -263.153748, -142.400177, -22.0657940, 137.646042, 130.208954,
            -84.9520569, -253.231140, -428.251190, -836.046326, -954.207153, -821.159607, -556.019653, -123.746704, 267.252045, 762.026184, 987.335266, 1322.41577, 1468.61487, 1218.09302, 1102.17468, 1035.27136, 903.178223, 734.693604, 452.896515, 217.177948,
            128.837372, -185.739426, -471.570496, -535.851379, -631.489197, -658.994568, -561.090515, -560.720825, -612.178467, -565.156982, -735.077148, -605.619385, -412.010498, -85.7889175, 255.370483, 574.118286, 612.100037, 506.903534, 192.718689, 15.4847460,
            -297.064789, -576.046570, -508.441193, -222.874634, 67.4066010, 357.049652, 682.486145, 959.688171, 1112.14697, 1190.11890, 1134.09167, 1197.30005, 1196.77747, 1268.81958, 1205.12695, 1113.60571, 803.309875, 599.889771, 288.529205, 249.904099,
            330.000763, 348.003815, 368.142548, 376.048798, 212.066925, -64.4699402, -314.513580, -582.113403, -939.588684, -1077.70801, -947.317749, -609.931274, -379.326385, -225.296219, -156.418961, -140.654587, -223.673309, -161.126129, 57.4517937, 481.723358,
            784.805542, 674.626282, 763.495972, 912.545349, 725.020813, 364.044067, 46.6318550, -198.413101, -83.2918396, -147.224442, -133.337921, -59.4149818, 23.4191399, 164.900299, 290.968811, 434.157379, 705.958008, 1174.41821, 1550.16687, 1666.35339,
            1506.08984, 1283.31909, 743.360474, 120.849915, -361.733459, -875.818237, -1064.90332, -1104.43848, -1036.79382, -917.302307, -689.847046, -653.249817, -719.806213, -885.375488, -788.384644, -504.664551, -153.451599, 264.495941, 410.872498, 691.459717,
            803.532288, 894.760010, 906.336548, 650.706299, 435.423584, 135.017578, -203.496735, -683.436646, -1041.36597, -1299.04492, -1227.72241, -1200.57397, -910.312500, -269.467621, 362.331512, 857.510376, 1242.54968, 1438.41736, 1328.53552, 1129.17114,
            666.904236, 431.578430, 263.163025, 213.459351, -18.0168476, -267.391724, -502.352783, -928.407532, -1163.81860, -1635.05078, -1658.75488, -1479.01746, -1145.84326, -582.514221, 135.019058, 854.921936, 1213.37891, 1476.83826, 1392.16150, 1246.75769,
            1091.04834, 937.014282, 540.853821, 144.443039, -254.662842, -694.914551, -927.411316, -1048.33044, -1020.22839, -783.363342, -339.995758, -96.1463776, 174.930511, 213.217529, 313.760620, 121.694778, 6.21601009, -113.318199, 145.521271, 451.906097,
            469.043457, 642.692566, 612.181091, 456.593231, 175.823532, -355.063751, -706.632507, -879.533875, -692.785339, -450.429779, -151.298981, 264.710938, 445.241943, 635.114868, 705.193848, 621.367798, 718.436157, 638.395996, 544.001282, 665.897949,
            709.703186, 623.509888, 383.871948, 136.148239, -458.447296, -685.334412, -670.229431, -780.159668, -781.440796, -847.862488, -1060.14343, -1358.82434, -1400.55920, -1565.47632, -1237.72461, -890.062622, -703.776428, -414.658325, -101.647682,
            225.809052, 153.896210, 46.9310684, -167.372116, -157.671356, 14.4361229, 118.911133, 169.702332, 314.788757, 501.626678, 482.325653, 414.521973, 366.982483, 52.3679810, 169.473969, 376.382904, 377.537140, 585.235596, 323.384491, 323.207245, 289.950745,
            146.411621, 175.999466, 483.613251, 634.592590, 719.582825, 940.653137, 707.542480, 286.391296, -419.223602, -1057.00220, -1254.94055, -1288.09814, -1346.48694, -1342.95679, -1213.62537, -1318.33508, -1177.94397, -1208.74292, -1017.86316, -701.729980,
            -409.209473, -132.240524, 79.8398361, 535.346558, 814.055603, 924.470337, 884.244507, 552.816711, 111.978973, -150.021240, -405.140564, 35.2445450, -16.6605186, -9.16959476, 5.88172913, 0.732177556, -5.56406879, -0.692634106, -0.0862214342, -4.37428141,
            8.05155659, 5.59535742, 0.857761145, 0.169312730, 3.57619309, 0.705901682, 3.68210983, 3.95765781, 0.924851477, 3.31332350, 3.87147737, -1.96845317, 2.43343282, 0.594578207, 0.145277590, 0.0315122791, 0.00683535403, 0.00148266216, 5.88227034,
            -2.27848148, -3.50786304, -0.528444171, -3.24422741, -0.218945190, -0.0147760902, -0.000997203169, -6.72988681e-05, -3.37499499, 10.0658493, 3.55135298, 0.0622216128, -0.00104091084, 1.74134893e-05, -2.91311807e-07, 4.87338081e-09, -3.68109682e-10,
            2.78050794e-11, 1.80332386, -0.136213645, -1.30805910, 0.173563674, -1.34916282, 0.179017633, -0.0301750489, 0.00508627854, -0.000857338484, -1.04508746, 0.184592590, -0.912068307, 0.161097571, -0.0284544770, 0.00450441753, -0.785152376, -0.660147667,
            0.104503088, -0.196723819, 0.207980156, -0.209354088, -0.158414856, -0.165640950, 0.193149656, 0.162692353, 0.165277839, -0.0119678490, 0.000866597751, -1.41433036, 0.102412350, -0.0105752135, 0.00109200832, -0.000112761991, 1.16439287e-05,
            -1.87709679e-06, 3.02603382e-07, -4.87821445e-08, -0.660956383, 0.760437012, -0.780599594, 0.158210814, -0.658541262, -0.0200174898, -0.145121709, -0.120610379, 0.172674537, 0.120220855, 0.127618581, -0.162572235, -0.121645622, 0.00720507791,
            -0.000426757208, 2.52768568e-05, -1.49714981e-06, -2.95575333e-08, 0.591256022, 0.0116728926, 0.000230452497, -0.152319551, 0.143011034, -0.143580750, 0.143545881, -0.148708358, -0.162101775, 0.147857979, -0.148510739, -0.156053767, 0.157313228,
            0.156017378, 0.156022727, -0.164391428, -0.150150120, 0.164130405, -0.164751783, 0.00765995169, 0.628844380, -0.0292374231, 0.00135936157, -2.14461725e-05, 3.38348769e-07, -5.33801048e-09, 8.42159259e-11, 1.53433570e-12, 2.79541676e-14, 5.09298888e-16,
            9.27895146e-18, 3.04800572e-19, 1.00122725e-20, -0.590371609, -0.0193928815, 0.144803748, 0.149423346, -0.141145393, -0.149320424, 0.144704774, -0.144773304, -0.149071068, -0.149134874, 0.000235811240, -3.72863440e-07, 5.89569615e-10, -9.32224241e-13,
            2.40323711e-14, -6.19544991e-16, 1.59716235e-17, -0.645910382, 0.209900171, 0.156031638, -0.179065019, 0.180514842, -0.190212175, -0.153922245, 0.187609062, -0.190906614, 0.189310402, -0.189158171, -0.153064609, -0.156506762, 0.173416883, -0.174019918,
            -0.161629915, 0.173599586, 0.0112208836, 0.000725279562, 4.68795915e-05, 3.03013644e-06, 4.69103753e-07, 0.645013511, 0.0998563170, 0.0154590318, 0.00263138697, 0.000447906263, 7.62411728e-05, 1.29775281e-05, -0.551019132, -0.0616038628, -0.00688730320,
            -0.000769999519, -3.12861448e-05, -1.27119938e-06, -5.16505914e-08, -2.09863504e-09, -0.482768208, 0.00100682245, -2.09974792e-06, 4.37906511e-09
        ];

        #[rustfmt::skip]
        let overlap_buf: Vec<f32> = vec![-1100.73206, -302.952087, -621.056702, -859.605164, -431.479248, -589.498840, -547.406677, -398.225830, -376.513184, -637.410950, 19.1916008, 563.179626, 315.718506, 136.099716, 21.8708096, 153.065552, -472.231415, -1320.12244, -1203.38867, -1320.74756, -1977.28296, -2085.62451, -1927.11890, -1638.04321, -813.263428, -351.079712, -599.614624, -814.685059, -322.241211, -320.594208, -299.860809, -128.416016, -156.443481, -627.559326, -402.451477, -270.737183, -388.990692, 33.5765305, 100.779968, 215.312302, -80.1882172, -101.931587, 268.805603, 159.354523, -187.031052, -134.435760, 418.012390, 444.497559, 659.483643, 601.999451, 347.478760, 205.690521, -39.5874481, -867.243896, -1121.57043, -709.199402, -727.358704, -548.686523, 22.6302395, 806.678528, 1197.71167, 1670.41333, 1731.20837, 1262.23755, 1009.94299, 749.770020, 816.701172, 485.910217, 486.419464, 701.587524, 1149.55957, 997.583679, 847.363403, 1245.18176, 1170.11169, 1140.35181, 1037.99976, 853.600891, 461.064850, 713.282288, 121.788918, -207.278854, 73.7572937, 91.0772018, -433.189728, -479.946350, -372.482819, -680.724792, -1085.11035, -1388.04297, -1681.23865, -1767.25415, -1333.58887, -1109.25281, -278.997681, 146.699539, 697.811768, 977.137512, 724.147583, 246.468155, 23.2934074, -493.916351, -839.799255, -590.953369, -858.909729, -469.833832, -224.661545, -236.605896, -435.203796, -329.346191, -5.22708845, -74.3638611, -251.496170, -291.481537, -351.589844, -356.068207, -437.146027, -813.977478, -903.577881, -927.534729, -980.804749, -709.496277, -489.506653, -375.107025, -367.567657, -500.205170, -799.333313, -824.126587, -863.780884, -758.307190, -223.683884, 268.538788, 361.559937, 970.060669, 1405.65613, 1026.59143, 829.295471, 817.487244, 548.422180, 429.084625, 430.868744, 269.390869, 164.718491, -224.270691, -556.754089, -932.212524, -1195.20020, -1351.43506, -1484.75256, -1154.14246, -980.858765, -816.249939, -536.335876, -496.967407, -511.936371, -785.894226, -735.576355, -366.783264, -126.517387, -174.710648, -81.9534149, -32.5626183, -325.666473, -745.051575, -896.061035, -736.110352, -512.909424, 60.3958206, 415.207397, 1057.53882, 1591.55493, 1724.94189, 1703.40845, 1439.63879, 1364.30640, 1052.97681, 884.408752, 545.856201, 309.600922, 64.2106934, -5.14267302, -273.215851, -259.974335, -184.051865, -155.397934, -19.3119144, -37.8418655, 143.899857, -15.9176846, -480.273224, -921.864929, -1198.84375, -1303.82715, -1283.84387, -902.437744, -569.331604, -155.738907, 229.943863, 270.602570, -153.938980, -358.804840, -340.129822, -493.189026, -134.018738, 54.3472748, 209.533997, 558.227783, 946.161682, 1006.85553, 926.835144, 614.287048, 431.745514, 351.467255, 243.072495, 306.459442, 385.298798, 494.323792, 695.139282, 855.356812, 1082.20740, 1251.83691, 1368.32312, 1309.72681, 935.405884, 599.451538, 35.6674271, -446.340515, -849.126770, -1127.67065, -1096.17041, -720.777405, -393.334412, -100.449348, 149.411224, 52.6758614, -285.212616, -690.674500, -1049.91748, -920.106873, -789.934692, -586.156860, -40.4311676, 435.512054, 966.683655, 1280.13513, 1196.11572, 963.974792, 679.680420, 368.705902, -49.4520569, -413.084442, -475.896637, -360.318329, -21.2329082, 45.9487000, 252.659576, 756.111206, 1107.50952, 1310.00110, 1373.33569, 1038.02332, 626.884644, 429.703369, 77.9613342, -237.231796, -239.379211, -165.359451, -365.100250, -476.436401, -653.155273, -934.191101, -1414.84326, -1714.17322, -1853.98022, -1678.57739, -1117.33740, -500.800812, 184.191727, 770.837463, 1284.04285, 1525.97534, 1384.99097, 1058.63892, 519.526123, 54.4562111, -372.841248, -974.807190, -1113.51709, -1058.34265, -854.338196, -680.580688, -459.454681, -133.861084, 214.333771, 563.529114, 561.533081, 469.786835, 332.447815, 154.138550, -127.256760, -263.153748, -142.400177, -22.0657940, 137.646042, 130.208954, -84.9520569, -253.231140, -428.251190, -836.046326, -954.207153, -821.159607, -556.019653, -123.746704, 267.252045, 762.026184, 987.335266, 1322.41577, 1468.61487, 1218.09302, 1102.17468, 1035.27136, 903.178223, 734.693604, 452.896515, 217.177948, 128.837372, -185.739426, -471.570496, -535.851379, -631.489197, -658.994568, -561.090515, -560.720825, -612.178467, -565.156982, -735.077148, -605.619385, -412.010498, -85.7889175, 255.370483, 574.118286, 612.100037, 506.903534, 192.718689, 15.4847460, -297.064789, -576.046570, -508.441193, -222.874634, 67.4066010, 357.049652, 682.486145, 959.688171, 1112.14697, 1190.11890, 1134.09167, 1197.30005, 1196.77747, 1268.81958, 1205.12695, 1113.60571, 803.309875, 599.889771, 288.529205, 249.904099, 330.000763, 348.003815, 368.142548, 376.048798, 212.066925, -64.4699402, -314.513580, -582.113403, -939.588684, -1077.70801, -947.317749, -609.931274, -379.326385, -225.296219, -156.418961, -140.654587, -223.673309, -161.126129, 57.4517937];

        #[rustfmt::skip]
        let data_out_ref: [f32; 768] = [
            -57.4517937, 161.126129, 223.673309, 140.654587, 156.418961, 225.296219, 379.326385, 609.931274, 947.317749, 1077.70801, 939.588684, 582.113403, 314.513580, 64.4699402, -212.066925, -376.048798, -368.142548, -348.003815, -330.000763, -249.904099, -288.529205, -599.889771, -803.309875, -1113.60571, -1205.12695, -1268.81958, -1196.77747, -1197.30005, -1134.09167, -1190.11890, -1112.14697, -959.688171, -682.486145, -357.049652, -67.4066010, 222.874634, 508.441193, 576.046570, 297.064789, -15.4847460, -192.718689, -506.903534, -612.100037, -574.118286, -255.370483, 85.7889175, 412.010498, 605.619385, 735.077148, 565.156982, 612.178467, 560.720825, 561.090515, 658.994568, 631.489197, 535.851379, 471.570496, 185.739426, -128.837372, -217.177948, -452.896515, -734.693604, -903.178223, -1035.27136, -1102.17468, -1218.09302, -1468.61487, -1322.41577, -987.335266, -762.026184, -267.252045, 123.746704, 556.019653, 821.159607, 954.207153, 836.046326, 428.251190, 253.231140, 84.9520569, -130.208954, -137.646042, 22.0657940, 142.400177, 263.153748, 127.256760, -154.138550, -332.447815, -469.786835, -561.533081, -563.529114, -214.333771, 133.861084, 459.454681, 680.580688, 854.338196, 1058.34265, 1113.51709, 974.807190, 372.841248, -54.4562111, -519.526123, -1058.63892, -1384.99097, -1525.97534, -1284.04285, -770.837463, -184.191727, 500.800812, 1117.33740, 1678.57739, 1853.98022, 1714.17322, 1414.84326, 934.191101, 653.155273, 476.436401, 365.100250, 165.359451, 239.379211, 237.231796, -77.9613342, -429.703369, -626.884644, -1038.02332, -1373.33569, -1310.00110, -1107.50952, -756.111206, -252.659576, -45.9487000, 21.2329082, 360.318329, 475.896637, 413.084442, 49.4520569, -368.705902, -679.680420, -963.974792, -1196.11572, -1280.13513, -966.683655, -435.512054, 40.4311676, 586.156860, 789.934692, 920.106873, 1049.91748, 690.674500, 285.212616, -52.6758614, -149.411224, 100.449348, 393.334412, 720.777405, 1096.17041, 1127.67065, 849.126770, 446.340515, -35.6674271, -599.451538, -935.405884, -1309.72681, -1368.32312, -1251.83691, -1082.20740, -855.356812, -695.139282, -494.323792, -385.298798, -306.459442, -243.072495, -351.467255, -431.745514, -614.287048, -926.835144, -1006.85553, -946.161682, -558.227783, -209.533997, -54.3472748, 134.018738, 493.189026, 340.129822, 358.804840, 153.938980, -270.602570, -229.943863, 155.738907, 569.331604, 902.437744, 1283.84387, 1303.82715, 1198.84375, 921.864929, 480.273224, 15.9176846, -143.899857, 37.8418655, 19.3119144, 155.397934, 184.051865, 259.974335, 273.215851, 5.14267302, -64.2106934, -309.600922, -545.856201, -884.408752, -1052.97681, -1364.30640, -1439.63879, -1703.40845, -1724.94189, -1591.55493, -1057.53882, -415.207397, -60.3958206, 512.909424, 736.110352, 896.061035, 745.051575, 325.666473, 32.5626183, 81.9534149, 174.710648, 126.517387, 366.783264, 735.576355, 785.894226, 511.936371, 496.967407, 536.335876, 816.249939, 980.858765, 1154.14246, 1484.75256, 1351.43506, 1195.20020, 932.212524, 556.754089, 224.270691, -164.718491, -269.390869, -430.868744, -429.084625, -548.422180, -817.487244, -829.295471, -1026.59143, -1405.65613, -970.060669, -361.559937, -268.538788, 223.683884, 758.307190, 863.780884, 824.126587, 799.333313, 500.205170, 367.567657, 375.107025, 489.506653, 709.496277, 980.804749, 927.534729, 903.577881, 813.977478, 437.146027, 356.068207, 351.589844, 291.481537, 251.496170, 74.3638611, 5.22708845, 329.346191, 435.203796, 236.605896, 224.661545, 469.833832, 858.909729, 590.953369, 839.799255, 493.916351, -23.2934074, -246.468155, -724.147583, -977.137512, -697.811768, -135.199768, 245.614944, 1151.49353, 1385.07361, 1759.30420, 1812.20630, 1663.18323, 1343.73047, 984.618774, 792.046753, 925.857178, 927.580322, 581.026733, 600.437378, 634.469543, 407.559479, 175.811768, 311.823944, -216.037903, -43.0698242, -41.0606689, -342.424683, -486.077881, -177.152222, -372.763184, -263.652588, 149.053833, 51.0789185, 127.542480, -47.8225098, -139.496521, -500.645386, -690.947205, -1069.21143, -994.788330, -488.812744, 39.2705078, 621.457214, 922.951721, 1551.30945, 1790.09680, 1503.43103, 997.687683, 474.350250, -15.1080475, -214.212250, -52.3386841, -87.0756226, 155.611328, 406.376648, 716.414978, 645.430420, 424.820892, -7.25564575, -63.4267120, -158.670135, -450.148315, -455.929138, -720.736755, -509.130524, -428.836914, -608.082886, -700.457703, -492.542175, -287.661926, -627.187927, -527.369751, -470.032898, -433.401672, -403.057190, -254.812256, 238.272034, 1453.46252, 1933.71509, 1553.52368, 1148.35059, 682.112061, 927.501831, 1409.98682, 453.946655, -335.862823, 193.999237, 446.375549, 548.126648, 689.820862, 907.026001, 1339.64075, 1460.35693, 1058.81238, 743.917969, 1114.64832, 917.064331, 832.561890, 686.481873, 389.619507, 971.762146, 912.918396, 652.113647, 692.906128, 638.682800, 567.495544, 528.147888, 307.256531, 819475.688, 277035.969, 1094507.25, 790711.062, 630615.312, 849609.500, 670456.062, 569068.188, 622698.500, -244808.875, 326561.656, -355782.438, -1008770.81, -971097.750, -744834.562, -1615913.50, -269952.688, -743593.000, -1749329.38, -731009.188, -1839170.38, -1517725.62, -1421553.25, -1779410.88, -1557539.75, -1338175.62, -1535312.25, -1232554.12, -1347174.88, -1381487.00, -1185615.75, -578788.250, -1020396.75, -188050.719, 360174.500, 530531.375, 1098763.62, 1428023.75, 956415.375, 1081592.50, 1089712.25, 870355.688, 956155.125, 868325.188, 1019159.69, 1116796.75, 1145269.25, 1170343.38, 2007772.25, 1371607.25, 2179838.00, 2600006.00, 1812972.12, 3672551.75, 2622886.50, 3177099.25, 2963201.25, 2588384.00, 2749745.50, 2765674.75, 2551026.75, 2632108.75, 3732580.75, 3034959.50, 4076172.50, 3528761.25, 3309165.00, 3298800.25, 3142333.00, 2923543.25, 2892460.50, 3926606.50, 2348683.00, 3862940.50, 2403406.00, 3337428.50, 2997019.50, 3395707.50, 3065949.25, 2803268.00, 3926365.75, 4092357.25, 4491079.50, 4689567.00, 3946449.75, 3558205.00, 3468584.50, 2856890.25, 2991488.00, 1909515.12, 2395676.00, 1696599.25, 1786599.00, 2028558.25, 1627216.75, 1610606.00, 1502016.12, 1361461.00, 1158504.00, 2150861.50, 1403408.38, 2370709.75, 104436.750, 766038.500, -688881.938, -537254.500, -46618.8125, -560961.000, -195013.125, -145960.234, 23663.9219, -339527.312, -1118654.62, -2200317.50, -2261829.25, -3185665.00, -2584854.25, -2482961.25, -2513502.50, -2136754.00, -2143831.75, -2175497.00, -2525898.75, -2559707.00, -2766703.50, -2561375.50, -2521871.00, -2294718.00, -2168761.00, -2907437.75, -2608313.00, -3296229.50, -3027591.50, -1900262.25, -2236765.75, -1437647.50, -403008.875, -80991.1875, 380873.312, 1021742.50, 460769.188, 468596.250, 1598472.38, 1315238.62, 2196942.00, 2279920.00, 2124166.00, 2380900.50, 2459912.75, 2237977.75, 2208940.50, 2373757.25, 2245730.50, 2436788.00, 2405353.50, 2418388.00, 2291536.00, 2306184.50, 3026546.00, 3549788.50, 3925542.25, 4336736.00, 2861303.75, 2026280.25, 1740807.12, 860103.375, 1185875.00, 1468695.00, 1459357.62, 1666279.25, 1704893.75, 448352.000, 443575.438, -505501.500, -754827.625, -430211.562, 358393.688, 253642.547, 961320.375, 1005779.94, -520309.125, -160994.188, -1177152.50, -1240221.75, -916170.250, -736870.000, -604980.125, -428315.125, -301213.188, -700039.750, -745706.375, -1004320.81, -1075185.50, -1130282.75, -835498.375, -796519.875, -647561.688, -497194.656, -654971.062, -645129.062, -734666.875, -757325.438, -790119.750, -633119.125, -555318.750, -457777.562, -317777.656, -333870.781, -323082.125, -377764.812, -411854.188, -482325.938, -465866.438, -462637.625, -421095.062, -370732.938, -340800.031, -319531.375, -336641.531, -336955.969, -366639.281, -355655.875, -347584.500, -324320.000, -262170.156, -208813.844, -173853.562, -143656.391, -145025.547, -160904.094, -153174.703, -149126.531, -133195.734, -109739.719, -83785.9297, -64251.4141, -50742.0977, -43369.2227, -46827.7422, -47289.4180, -46724.0273, -43296.3984, -34601.6562, -21281.4863, -12013.0801, -5039.82666, -979.187988, -1784.98413, -2894.24072, -4152.68994, -4418.26367, -27485.9941, -47785.3047, -83072.6250, -99378.2656, -106419.906, -98973.5703, -86413.6328, -85745.7812, -83620.6250, -65129.0742, -70097.5391, -36962.5820, -37470.9141, -9871.24609, -5807.84180, 2072.03467, -9943.84863, -15795.2012, -20145.3379, -24032.1953, -8387.46289, -5289.48584, 4261.21680, 31306.4844, 59378.7734, 73890.9922, 90830.9453, 84486.3672, 70746.5859, 67764.0469, 66831.8359, 70341.4453, 76553.4062, 86681.6328, 114422.906, 116850.938, 134086.938, 133474.438, 117637.570, 111412.859, 102127.406, 99970.8125, 100093.414, 106078.961, 108267.344, 108743.586, 108847.141, 104053.922, 96461.4297, 89435.2578, 83035.8125, 100438.219, 98663.9062, 113527.891, 140171.906, 129435.398, 142114.062, 138009.172, 124398.766, 117719.719, 113595.703, 113199.156, 104493.797, 108981.422, 132504.438, 132757.578, 146580.562, 147004.484, 131100.344, 139388.766, 131947.672, 139646.594, 137597.578, 133280.719, 132975.438, 123906.734, 125767.391, 122016.805, 117788.750, 112932.781, 104108.234, 97578.6094, 91223.0781, 89185.6172, 85416.0781, 80216.2812, 75305.1641, 48908.5820, 19591.5625, -3326.80371, -46875.0156, -53459.8906, -62929.7344, -65224.5742, -58834.7070, -59490.0586, -58309.7500, -69159.1484, -94999.0938, -108177.180, -154109.156, -166630.188, -176372.625, -176486.875, -157880.750, -148354.750, -136709.422, -139090.000, -140465.031, -165717.516, -179227.906, -214121.656, -223509.469, -225455.641, -238835.875, -217114.891, -217252.594, -206850.391, -197140.406, -196799.047, -194992.484, -222272.297, -222202.719, -256170.000, -259777.969, -252478.531,
        ];

        let _fac_w_zir: [f32; 2 * LFAC_768] = [0.0f32; 2 * LFAC_768];
        let mut output: [f32; FRAMESIZE] = [0.0f32; FRAMESIZE];
        #[rustfmt::skip]
        let a: [f32; M_LP_FILTER_ORDER] = [0.103927135, -0.116784155, -0.416037619, -0.197623134, -0.124277353, 0.0487725735, 0.157457113, -0.0444087982, -0.163494110, -0.156350851, -0.117252111, 0.0543696880, -0.0467225313, 0.0241704583, 0.0468326211, 0.0681343079, ];

        let is_short_fac: bool = false;
        let last_frame_lost: bool = false;
        let is_fd_fac: bool = false;
        let prev_window_shape: WindowShape = WindowShape::Sine;
        let k: usize = 0;

        // MDCT
        let mut mdct = mdct::Mdct::new(384).unwrap();
        mdct.prev_wrs = Some(&window_tables::SINE_WINDOW_192);
        mdct.prev_nr = 288;
        mdct.prev_tl = FRAMESIZE;
        mdct.overlap = overlap_buf;

        // Assign FAC memory (frame index 0)
        let frame_idx: usize = 0; /* first frame, i.e. the TCX80 frame */
        let lpd_modes = [
            LpdMode::Acelp,
            LpdMode::Acelp,
            LpdMode::Acelp,
            LpdMode::Tcx20,
        ];
        fac_data.assign_memory(Some(&lpd_modes), frame_idx);
        assert!(fac_data.mem_state == 1);
        assert!(fac_data.data_index[frame_idx] == 0);

        let (samples_written, _) = fac_data.mdct2acelp(
            &mut data_in,
            &mut output,
            &None,
            &a,
            &mut mdct,
            prev_window_shape,
            k,
            is_short_fac,
            last_frame_lost,
            is_fd_fac,
        );

        assert!(samples_written == FRAMESIZE / 2);

        for (i, (c, r)) in izip!(&output[..samples_written], data_out_ref).enumerate() {
            assert!(
                (1.0 - (c / r)).abs() < (2_i32.pow(15) as f32).recip(),
                "Value {} differs too much: value is {}, ref is {}, deviation is {}.",
                i,
                c,
                r,
                (1.0 - (c / r)).abs()
            );
        }
    }

    #[test]
    fn acelp2mdct_short_frames_w_fac() {
        const FRAMESIZE_768: usize = 768;
        const LFAC_SHORT_768: usize = 48;

        // MDCT
        let mut mdct = mdct::Mdct::new(384).unwrap(); // Also fills overlap buffer with zeroes.

        {
            let mut fac_data: FacData = Default::default();

            // Buffers
            #[rustfmt::skip]
            let mut data: [f32; FRAMESIZE_768] = [
                -34716.4883, -59610.6875, 0.00000000, -34716.4883, 0.00000000, 0.00000000,
                13777.2471, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                13777.2471, 13777.2471, 13777.2471, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                24548.2637, -61857.7461, -9741.98438, 24548.2637, -9741.98438, 42151.1211,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, -13777.2471, 13777.2471,
                -9741.98438, -9741.98438, 24548.2637, 0.00000000, 9741.98438, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 9741.98438, -9741.98438, 0.00000000, 6888.62354,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                -6888.62354, 6888.62354, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                24548.2637, 61857.7461, -61857.7461, -24548.2637, 42151.1211, 61857.7461,
                -24548.2637, -24548.2637, 13777.2471, 0.00000000, -13777.2471, 13777.2471,
                9741.98438, -61857.7461, -24548.2637, 0.00000000, 9741.98438, -9741.98438,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, -9741.98438, 0.00000000, 17358.2441, 6888.62354,
                -6888.62354, 0.00000000, 6888.62354, 0.00000000, 0.00000000, 6888.62354,
                0.00000000, -17358.2441, -6888.62354, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                -24548.2637, 106214.156, 24548.2637, 42151.1211, 9741.98438, 83292.7969,
                -24548.2637, 9741.98438, -13777.2471, -34716.4883, -13777.2471, 13777.2471,
                42151.1211, 42151.1211, 24548.2637, 42151.1211, 9741.98438, 0.00000000,
                -9741.98438, -9741.98438, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, -9741.98438, -9741.98438, -24548.2637, 0.00000000, -6888.62354,
                0.00000000, 6888.62354, 6888.62354, 6888.62354, -6888.62354, -6888.62354,
                -17358.2441, -17358.2441, -17358.2441, -6888.62354, 0.00000000, 6888.62354,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                -43740.0312, -92242.2422, 17358.2441, -29805.3438, -17358.2441, 58896.9062,
                -58896.9062, 75104.7578, 0.00000000, -6888.62354, -17358.2441, -29805.3438,
                -17358.2441, -6888.62354, 75104.7578, 17358.2441, 6888.62354, 17358.2441,
                0.00000000, 0.00000000, 0.00000000, -6888.62354, -6888.62354, 0.00000000,
                -6888.62354, -6888.62354, 6888.62354, -17358.2441, 29805.3438, -6888.62354,
                17358.2441, 0.00000000, 0.00000000, 0.00000000, -6888.62354, -6888.62354,
                0.00000000, -17358.2441, -6888.62354, 6888.62354, 6888.62354, 0.00000000,
                0.00000000, 0.00000000, -4096.00000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 4096.00000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, -512.000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                43740.0312, -58896.9062, -43740.0312, 43740.0312, -17358.2441, 29805.3438,
                -17358.2441, -110217.977, 43740.0312, -17358.2441, 17358.2441, -6888.62354,
                0.00000000, 75104.7578, -43740.0312, -17358.2441, 17358.2441, -6888.62354,
                6888.62354, -6888.62354, 0.00000000, 0.00000000, 0.00000000, -6888.62354,
                6888.62354, 0.00000000, -6888.62354, 17358.2441, -29805.3438, -17358.2441,
                17358.2441, -6888.62354, 6888.62354, -6888.62354, 0.00000000, 0.00000000,
                -6888.62354, 17358.2441, -17358.2441, -6888.62354, 6888.62354, -6888.62354,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 4096.00000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 512.000000, 512.000000, 0.00000000, 0.00000000,
                0.00000000, 861.077942, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                861.077942, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                6888.62354, 110217.977, 29805.3438, -17358.2441, -17358.2441, 17358.2441,
                75104.7578, 17358.2441, -17358.2441, -58896.9062, 17358.2441, 43740.0312,
                -17358.2441, -75104.7578, -110217.977, 17358.2441, 29805.3438, 6888.62354,
                -17358.2441, -6888.62354, 6888.62354, 0.00000000, -6888.62354, 0.00000000,
                6888.62354, 0.00000000, -17358.2441, -6888.62354, 29805.3438, 43740.0312,
                17358.2441, -6888.62354, -6888.62354, 6888.62354, 6888.62354, -6888.62354,
                -17358.2441, 6888.62354, 17358.2441, 29805.3438, 0.00000000, -6888.62354,
                -6888.62354, 6888.62354, 4096.00000, -4096.00000, 0.00000000, 0.00000000,
                4096.00000, 0.00000000, 0.00000000, 0.00000000, 4096.00000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, -512.000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 512.000000, -512.000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, -861.077942, 0.00000000, 0.00000000, 861.077942,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 512.000000, -512.000000,
                -61857.7461, -9741.98438, -9741.98438, 0.00000000, 0.00000000, -9741.98438,
                -24548.2637, 42151.1211, -89315.1094, -20642.5469, 0.00000000, -8192.00000,
                -8192.00000, -8192.00000, 20642.5469, -52015.9570, -14596.4844, -5792.61865,
                -14596.4844, 5792.61865, 0.00000000, 5792.61865, 5792.61865, 5792.61865,
                5792.61865, 14596.4844, 14596.4844, 14596.4844, 25063.2070, 14596.4844,
                14596.4844, 25063.2070, 5792.61865, 5792.61865, 5792.61865, 5792.61865,
                8679.12207, 8679.12207, 3444.31177, -3444.31177, 8679.12207, 8679.12207,
                3444.31177, 3444.31177, 3444.31177, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, -3444.31177, -3444.31177, 0.00000000,
                -3444.31177, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, -608.874023,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 1217.74805, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                ];
            let mut fac_w_zir: [f32; 2 * LFAC_SHORT_768] = [0.0f32; 2 * LFAC_SHORT_768];
            let mut output: [f32; FRAMESIZE_768] = [0.0f32; FRAMESIZE_768];
            #[rustfmt::skip]
            let a: [f32; M_LP_FILTER_ORDER] = [
                -0.703607619, -0.200517356, 0.175958753, -0.0791246891,
                -0.0725680590,-0.153586507,-0.0247244835, 0.286121130,
                -0.0588548183,-0.140276432, 0.0842124224,-0.000352263451,
                -0.118322134, -0.0423823595,0.0354135633, 0.145339668,
            ];
            let a_old: [f32; M_LP_FILTER_ORDER] = [0.0f32; M_LP_FILTER_ORDER];
            let output_offset = 0;
            #[rustfmt::skip]
            let overlap_buf: Vec<f32> = vec![
                -3216.30273, -1343.15503, -1112.72021, 849.813232, 1690.72607, 4203.53711,
                5360.15234, 8154.37695, 9771.63086, 10828.4609, 14080.3594, 15650.8486,
                21039.0449, 23140.9316, 26708.5078, 28975.6250, 29118.5273, 28753.9238,
                26097.4004, 21932.9102, 19095.2695, 15266.2344, 12919.7852, 10650.8037,
                9815.66797, 8334.48145, 9992.52539, 9681.31445, 10333.6895, 9819.37305,
                9111.61914, 6425.14355, 3131.83496, -2166.29712, -6818.53418, -11395.3535,
                -14718.3711, -17656.4453, -20942.2324, -21439.1270, -23608.3164, -23220.6367,
                -24189.7852, -23271.3086, -23775.2461, -22336.8672, -22023.9395, -21942.6797,
                -21737.9922, -21212.0000, -20441.7520, -19762.2129, -18755.9902, -18087.7520,
                -16812.1289, -15811.8789, -14254.9727, -13077.6094, -11577.4326, -10279.0830,
                -8755.09570, -7273.75244, -3896.58521, -1518.87109, -900.365723, 3050.17041,
                4981.88525, 6912.31104, 8588.10059, 7399.57617, 6191.99512, 4614.33203,
                2385.04272, 2460.11475, 2031.07776, 1623.61841, 1153.66711, 718.667603,
                920.319336, 3446.22632, 3560.53955, 6047.85938, 6117.87256, 6230.12402,
                6561.80078, 6696.79980, 7122.35254, 7020.54541, 7011.02051, 7050.96533,
                7051.44775, 7074.01904, 4512.20557, 4173.83838, 1507.63684, 3217.14185,
                5519.66357, 7395.56641, 9969.40820, 10132.3223, 10685.5684, 8861.16113,
                6692.84375, 4209.29785, 624.330078, 1832.84326, 864.951050, 2034.36597,
                -1037.25024, -1973.29944, -1890.69800, -2284.52197, -37.4879150, -437.299225,
                -4156.35010, -7450.41699, -10641.2852, -16804.0625, -18138.9219, -23054.4746,
                -27821.2949, -30276.5508, -34898.7656, -36834.7344, -38733.2773, -40341.4570,
                -41768.6484, -43258.3789, -40448.7734, -37394.1758, -30053.8125, -25990.8789,
                -25372.0117, -23231.5977, -25263.0273, -22476.8047, -19908.1660, -18279.9375,
                -20537.3906, -18917.0254, -21025.7734, -14160.1787, -12401.3408, -6196.04199,
                -4318.62695, -2956.05029, -4624.34473, -6455.46680, -11876.6777, -14423.2480,
                -17814.1484, -14103.8594, -14631.7441, -11536.7637, -11023.2168, -10862.0098,
                -8889.67090, -8215.73730, -7051.55371, -7049.52295, -7169.90234, -7165.86670,
                -6585.57324, -6255.27344, -5055.53467, -8554.72461, -11106.4883, -14450.3477,
                -18099.6719, -13930.1328, -14982.6338, -11304.1787, -7976.75293, -8641.52441,
                -3449.96704, -2582.92285, -1212.00854, 159.233582, 321.583771, -2553.54077,
                -1536.01831, -5223.72168, -3963.20386, 452.114502, 703.652710, 5493.73242,
                5581.28418, 6666.95996, 7922.58887, 8989.84277, 9979.11816, 10440.8262,
                10112.5938, 10214.3359, 10160.9375, 10681.6631, 11201.4795, 11610.6133,
                6462.04395, 1332.23755, -9404.97070, -14690.0039, -20927.2129, -22061.4062,
                -18581.3535, -22030.6543, -18474.8438, -20626.3145, -15212.7227, -15667.2773,
                -10066.5293, -10777.9736, -11408.8828, -23411.5312, -27566.8594, -37923.7031,
                -42213.3125, -41875.3398, -37485.1445, -32989.3906, -29661.0156, -23725.0664,
                -22788.2305, -14394.5156, -13624.0332, -5821.72461, -5943.61670, -5146.64941,
                1193.39111, 8127.24512, 16246.6152, 30529.0586, 32630.3008, 41890.2383,
                44826.0234, 48607.7734, 53125.6719, 54965.8203, 57684.3867, 58471.5430,
                59472.3438, 60451.0859, 61730.6250, 62770.3906, 63642.5664, 63188.9766,
                62714.6328, 60910.8281, 59646.1641, 52089.9297, 45107.4609, 37468.3242,
                28922.2441, 25909.4316, 21713.7461, 17744.5879, 7138.91016, 16470.5117,
                -1022.56152, 4520.67188, -5779.92480, -14149.6182, -18348.7559, -22192.0625,
                -24286.2852, -28287.1797, -30519.4102, -32670.1641, -33804.5117, -34361.2422,
                -34000.0508, -35273.2969, -34648.8086, -36058.0000, -35260.9219, -35801.0625,
                -34826.1055, -33496.1445, -32294.1250, -30835.9766, -29544.7695, -27745.6738,
                -32051.0059, -35523.2891, -45972.7188, -49889.8750, -55154.2773, -53224.3359,
                -52359.1875, -51528.0938, -50787.2422, -49445.1953, -40545.3711, -38151.5234,
                -28476.8477, -27222.5078, -25570.5762, -24869.5156, -22691.0840, -20013.1152,
                -16932.0820, -14028.0410, -11410.5273, -15388.8457, -18272.6074, -27356.9219,
                -30122.3320, -33897.5078, -32306.6758, -31630.0117, -25945.8516, -26389.6562,
                -18813.5449, -16993.3008, -14166.6641, -6136.93164, -5741.31445, 8189.48242,
                13212.1387, 22676.8613, 24229.6367, 21939.9844, 23794.0312, 20542.5176,
                23410.4844, 23869.8477, 24074.7422, 23373.9375, 22635.8184, 23327.7949,
                24139.8359, 25210.3652, 26176.8945, 24599.3633, 22880.9141, 20372.5547,
                18489.8008, 17238.4609, 15958.5195, 10159.7305, 8538.55762, 1531.21143,
                -4338.21875, -5306.76807, -10624.8252, -11434.0117, -13398.1680, -14123.8984,
                -15951.5000, -16495.2344, -16361.7285, -16869.7891, -16937.5742, -17227.2109,
                -18289.7070, -18830.2363, -18969.5781, -18437.8008, -17804.6113, -17163.4688,
                -16126.8018, -15460.8848, -9664.94727, -8823.65234, -2208.14600, -1563.31934,
                -4964.09131, -9023.87793, -17593.9824, -21003.2539, -25581.3555, -24065.1777,
                -24067.3496, -19157.0762, -14487.2266, -8617.19922, -15727.8633, -6919.45264,
                -21333.8828, -5195.28906, -7916.90234, 8753.11914, 5540.21729, 10747.4658,
            ];
            #[rustfmt::skip]
            let fac_buf: [f32; 2 * LFAC_SHORT_768] = [
                -2.47091126, 2.47091126, 2.47091126, -7.41273403, 7.41273403, 32.1218452,
                56.8309593, -2.47091126, -7.41273403, -7.41273403, 17.2963791, 46.9473152,
                17.2963791, 7.41273403, 27.1800232, 7.41273403, 0.00000000, -9.88364506,
                4.94182253, -44.4764023, -4.94182253, -14.8254681, 24.7091122, -4.94182253,
                2.47091126, -22.2382011, 2.47091126, -17.2963791, -7.41273403, -12.3545561,
                2.47091126, -7.41273403, 0.00000000, 4.94182253, 9.88364506, -19.7672901,
                0.00000000, -4.94182253, 9.88364506, -9.88364506, -12.3545561, -12.3545561,
                17.2963791, -2.47091126, -7.41273403, -17.2963791, -17.2963791, -17.2963791,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
                0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000, 0.00000000,
            ];

            #[rustfmt::skip]
            let old_syn_mem = [
                -11457.7451, -9039.34668, -11299.1436, -6669.85596,
                -7703.02881, -2791.27783, -1460.41455, 1234.11475,
                -9868.16797, 3775.49463, -16628.6543, 9311.75098,
                -4384.10547, 14136.6123, -411.903809, 6980.11816,
            ];

            //let is_short_fac: bool = false;
            let last_frame_lost: bool = false;
            let is_fd_fac: bool = true;
            let last_lpd_mode = LpdMode::Acelp;
            //let prev_window_shape: WindowShape = WindowShape::Sine;
            let gain = 1.0;
            let k: usize = 0;
            let n_spec = 8;
            let tl = FRAMESIZE_768 / MAX_WINDOWS;
            fac_data.data_0[..2 * LFAC_SHORT_768].copy_from_slice(&fac_buf);

            let mdct_ref = &mut mdct;
            mdct_ref.prev_wrs = Some(&window_tables::SINE_WINDOW_96);
            mdct_ref.prev_nr = 0;
            mdct_ref.prev_tl = 0;
            mdct_ref.ov_offset = 384;
            mdct_ref.overlap = overlap_buf;

            let wrs = &window_tables::SINE_WINDOW_96;

            // Assign FAC memory (frame index 0)
            let frame_idx: usize = 0; /* first frame, i.e. the TCX80 frame */
            let lpd_modes = [
                LpdMode::Undef,
                LpdMode::NotLpd,
                LpdMode::NotLpd,
                LpdMode::NotLpd,
            ];
            fac_data.assign_memory(Some(&lpd_modes), frame_idx);
            assert!(fac_data.mem_state == 9);
            assert!(fac_data.data_index[frame_idx] == 8);

            // ACELP
            const ACELP_BUF_OFFSET: usize = PIT_MAX_MAX + L_INTERPOL;
            let mut old_syn_mem_w_offset = [0.0f32; 428 + 1 + M_LP_FILTER_ORDER];
            let mut acelp_data = acelp::AcelpData::new();
            let mut old_exc_mem = [0.0f32; ACELP_BUF_OFFSET];
            let acelp_lpd_modes = [LpdMode::TcxTdConceal, LpdMode::NotLpd, LpdMode::NotLpd];
            acelp_data.init(48000);
            old_syn_mem_w_offset[ACELP_BUF_OFFSET + 1..ACELP_BUF_OFFSET + M_LP_FILTER_ORDER + 1]
                .copy_from_slice(&old_syn_mem);

            acelp_data.prepare_internal_mem(
                &mut old_exc_mem,
                &old_syn_mem_w_offset,
                &acelp_lpd_modes,
                &a,
                &a_old,
                768,
                true,
            );
            acelp_data.de_emph_mem = 10747.4658;

            let (samples_written, _fac_zir_offset) = fac_data.acelp2mdct(
                &mut data,
                &mut output,
                output_offset,
                Some(&mut fac_w_zir),
                &a,
                mdct_ref,
                &mut acelp_data,
                wrs,
                gain,
                k,
                tl,
                last_lpd_mode,
                n_spec > 1,
                last_frame_lost,
                is_fd_fac,
            );

            assert!(samples_written == FRAMESIZE_768);

            #[rustfmt::skip]
            let data_out_ref = [
                -3216.30273438, -1343.15502930, -1112.72021484, 849.81323242, 1690.72607422,
                4203.53710938, 5360.15234375, 8154.37695312, 9771.63085938, 10828.46093750,
                14080.35937500, 15650.84863281, 21039.04492188, 23140.93164062, 26708.50781250,
                28975.62500000, 29118.52734375, 28753.92382812, 26097.40039062, 21932.91015625,
                19095.26953125, 15266.23437500, 12919.78515625, 10650.80371094, 9815.66796875,
                8334.48144531, 9992.52539062, 9681.31445312, 10333.68945312, 9819.37304688,
                9111.61914062, 6425.14355469, 3131.83496094, -2166.29711914, -6818.53417969,
                -11395.35351562, -14718.37109375, -17656.44531250, -20942.23242188,
                -21439.12695312, -23608.31640625, -23220.63671875, -24189.78515625,
                -23271.30859375, -23775.24609375, -22336.86718750, -22023.93945312,
                -21942.67968750, -21737.99218750, -21212.00000000, -20441.75195312,
                -19762.21289062, -18755.99023438, -18087.75195312, -16812.12890625,
                -15811.87890625, -14254.97265625, -13077.60937500, -11577.43261719,
                -10279.08300781, -8755.09570312, -7273.75244141, -3896.58520508, -1518.87109375,
                -900.36572266, 3050.17041016, 4981.88525391, 6912.31103516, 8588.10058594,
                7399.57617188, 6191.99511719, 4614.33203125, 2385.04272461, 2460.11474609,
                2031.07775879, 1623.61840820, 1153.66711426, 718.66760254, 920.31933594,
                3446.22631836, 3560.53955078, 6047.85937500, 6117.87255859, 6230.12402344,
                6561.80078125, 6696.79980469, 7122.35253906, 7020.54541016, 7011.02050781,
                7050.96533203, 7051.44775391, 7074.01904297, 4512.20556641, 4173.83837891,
                1507.63684082, 3217.14184570, 5519.66357422, 7395.56640625, 9969.40820312,
                10132.32226562, 10685.56835938, 8861.16113281, 6692.84375000, 4209.29785156,
                624.33007812, 1832.84326172, 864.95104980, 2034.36596680, -1037.25024414,
                -1973.29943848, -1890.69799805, -2284.52197266, -37.48791504, -437.29922485,
                -4156.35009766, -7450.41699219, -10641.28515625, -16804.06250000, -18138.92187500,
                -23054.47460938, -27821.29492188, -30276.55078125, -34898.76562500,
                -36834.73437500, -38733.27734375, -40341.45703125, -41768.64843750,
                -43258.37890625, -40448.77343750, -37394.17578125, -30053.81250000,
                -25990.87890625, -25372.01171875, -23231.59765625, -25263.02734375,
                -22476.80468750, -19908.16601562, -18279.93750000, -20537.39062500,
                -18917.02539062, -21025.77343750, -14160.17871094, -12401.34082031, -6196.04199219,
                -4318.62695312, -2956.05029297, -4624.34472656, -6455.46679688, -11876.67773438,
                -14423.24804688, -17814.14843750, -14103.85937500, -14631.74414062,
                -11536.76367188, -11023.21679688, -10862.00976562, -8889.67089844, -8215.73730469,
                -7051.55371094, -7049.52294922, -7169.90234375, -7165.86669922, -6585.57324219,
                -6255.27343750, -5055.53466797, -8554.72460938, -11106.48828125, -14450.34765625,
                -18099.67187500, -13930.13281250, -14982.63378906, -11304.17871094, -7976.75292969,
                -8641.52441406, -3449.96704102, -2582.92285156, -1212.00854492, 159.23358154,
                321.58377075, -2553.54077148, -1536.01831055, -5223.72167969, -3963.20385742,
                452.11450195, 703.65270996, 5493.73242188, 5581.28417969, 6666.95996094,
                7922.58886719, 8989.84277344, 9979.11816406, 10440.82617188, 10112.59375000,
                10214.33593750, 10160.93750000, 10681.66308594, 11201.47949219, 11610.61328125,
                6462.04394531, 1332.23754883, -9404.97070312, -14690.00390625, -20927.21289062,
                -22061.40625000, -18581.35351562, -22030.65429688, -18474.84375000,
                -20626.31445312, -15212.72265625, -15667.27734375, -10066.52929688,
                -10777.97363281, -11408.88281250, -23411.53125000, -27566.85937500,
                -37923.70312500, -42213.31250000, -41875.33984375, -37485.14453125,
                -32989.39062500, -29661.01562500, -23725.06640625, -22788.23046875,
                -14394.51562500, -13624.03320312, -5821.72460938, -5943.61669922, -5146.64941406,
                1193.39111328, 8127.24511719, 16246.61523438, 30529.05859375, 32630.30078125,
                41890.23828125, 44826.02343750, 48607.77343750, 53125.67187500, 54965.82031250,
                57684.38671875, 58471.54296875, 59472.34375000, 60451.08593750, 61730.62500000,
                62770.39062500, 63642.56640625, 63188.97656250, 62714.63281250, 60910.82812500,
                59646.16406250, 52089.92968750, 45107.46093750, 37468.32421875, 28922.24414062,
                25909.43164062, 21713.74609375, 17744.58789062, 7138.91015625, 16470.51171875,
                -1022.56152344, 4520.67187500, -5779.92480469, -14149.61816406, -18348.75585938,
                -22192.06250000, -24286.28515625, -28287.17968750, -30519.41015625,
                -32670.16406250, -33804.51171875, -34361.24218750, -34000.05078125,
                -35273.29687500, -34648.80859375, -36058.00000000, -35260.92187500,
                -35801.06250000, -34826.10546875, -33496.14453125, -32294.12500000,
                -30835.97656250, -29544.76953125, -27745.67382812, -32051.00585938,
                -35523.28906250, -45972.71875000, -49889.87500000, -55154.27734375,
                -53224.33593750, -52359.18750000, -51528.09375000, -50787.24218750,
                -49445.19531250, -40545.37109375, -38151.52343750, -28476.84765625,
                -27222.50781250, -25570.57617188, -24869.51562500, -22691.08398438,
                -20013.11523438, -16932.08203125, -14028.04101562, -11410.52734375,
                -15388.84570312, -18272.60742188, -27356.92187500, -30122.33203125,
                -33897.50781250, -32306.67578125, -31630.01171875, -25945.85156250,
                -26389.65625000, -18813.54492188, -16993.30078125, -14166.66406250, -6136.93164062,
                -5741.31445312, 8189.48242188, 13212.13867188, 22676.86132812, 24229.63671875,
                21939.98437500, 23794.03125000, 20542.51757812, 23410.48437500, 23869.84765625,
                24074.74218750, 23373.93750000, 22635.81835938, 23327.79492188, 24139.83593750,
                25210.36523438, 26176.89453125, 24599.36328125, 22880.91406250, 20372.55468750,
                18489.80078125, 17238.46093750, 15958.51953125, 10159.73046875, 8538.55761719,
                1531.21142578, -4338.21875000, -5306.76806641, -10624.82519531, -11434.01171875,
                -13398.16796875, -14123.89843750, -15951.50000000, -16495.23437500,
                -16361.72851562, -16869.78906250, -16937.57421875, -17227.21093750,
                -18289.70703125, -18830.23632812, -18969.57812500, -18437.80078125,
                -17804.61132812, -17163.46875000, -16126.80175781, -15460.88476562, -9664.94726562,
                -8823.65234375, -2208.14599609, -1563.31933594, -4964.09130859, -9023.87792969,
                -17593.98242188, -21003.25390625, -25581.35546875, -24065.17773438,
                -24067.34960938, -19157.07617188, -14487.22656250, -8617.19921875, -15727.86328125,
                -6919.45263672, -21333.88281250, -5195.28906250, -7916.90234375, 8753.11914062,
                5540.21728516, 10747.46582031, 10904.77148438, 8920.32910156, 12145.61621094,
                4290.37597656, 5739.79101562, -3249.41845703, 3197.98803711, -1850.60937500,
                1430.68774414, -1549.30285645, -3333.16455078, -5984.83007812, -6079.80810547,
                -7442.38769531, -5681.14453125, -4369.41210938, -717.68212891, 901.66772461,
                1681.78222656, 858.76647949, -2251.25122070, -2821.27270508, -5030.67968750,
                -5321.37255859, -5637.17626953, -5765.17529297, -5942.64111328, -5996.70605469,
                -5863.20996094, -5519.02978516, -5042.16796875, -4631.39648438, -4390.60009766,
                -4021.90698242, -3872.71777344, -3561.52197266, -2974.46655273, -2633.76318359,
                -2111.10693359, -1787.04565430, -919.03643799, -610.58917236, 166.28735352,
                742.56311035, 729.36029053, 847.74975586, 765.75634766, 681.78247070, 453.36587524,
                381.59988403, 268.89172363, 194.15153503, 140.41760254, 112.88763428, 91.41764832,
                92.41942596, 144.26089478, 213.41265869, 283.52090454, 347.69778442, 375.48577881,
                345.04663086, 284.70440674, 250.92840576, 270.30847168, 295.95056152, 263.57574463,
                167.69918823, 100.32476044, 160.26448059, 342.20812988, 533.92376709, 632.63787842,
                644.52697754, 653.20385742, 709.81536865, 781.09735107, 810.98876953, 799.26379395,
                785.98931885, 771.68408203, 702.82495117, 560.88781738, 433.79904175, 446.60821533,
                611.81262207, 787.60638428, 820.53942871, 716.28979492, 629.38525391, 676.33074951,
                797.28308105, 835.01373291, 729.30847168, 583.52117920, 529.16107178, 575.00103760,
                636.43664551, 695.15423584, 850.64184570, 1175.08142090, 1556.89343262,
                1762.14440918, 1671.82397461, 1411.97802734, 1206.08056641, 1118.92431641,
                1000.43017578, 693.52160645, 246.97238159, -122.65390015, -258.38391113,
                -195.50631714, -62.14505005, 83.89193726, 255.59448242, 401.35855103, 367.09289551,
                56.34278870, -409.09268188, -772.56195068, -915.12335205, -993.53289795,
                -1250.51916504, -1700.35034180, -2063.86669922, -2044.92114258, -1638.93640137,
                -1130.28161621, -802.09521484, -697.94458008, -672.55737305, -610.00427246,
                -527.11608887, -482.10986328, -469.28768921, -457.21948242, -486.45089722,
                -641.94262695, -902.97247314, -1083.29431152, -993.81988525, -667.33819580,
                -364.79150391, -308.21542358, -489.10125732, -676.92742920, -656.15631104,
                -482.40399170, -415.59652710, -624.26904297, -1000.26489258, -1282.33105469,
                -1323.58581543, -1193.57983398, -1035.80700684, -895.64373779, -727.67431641,
                -527.38488770, -376.31878662, -336.12515259, -353.64907837, -326.78393555,
                -249.48348999, -229.91731262, -339.14834595, -483.74499512, -481.53018188,
                -262.29333496, 51.23181152, 281.94714355, 384.45257568, 455.00891113, 570.11572266,
                668.96655273, 625.18017578, 406.87155151, 120.81411743, -105.07278442,
                -244.60433960, -355.25952148, -463.27658081, -511.60034180, -433.18884277,
                -245.43754578, -42.94615173, 95.03002167, 161.35394287, 181.30435181, 148.36033630,
                33.06675720, -146.53076172, -296.92413330, -318.43414307, -198.66430664,
                -17.22674561, 147.79951477, 301.32794189, 503.49078369, 768.02960205,
                1013.17327881, 1129.23559570, 1086.80603027, 961.13842773, 849.97619629,
                787.24530029, 749.46154785, 726.58642578, 748.54669189, 828.61926270, 910.81286621,
                914.13134766, 833.08666992, 759.22845459, 776.21649170, 851.28247070, 867.76666260,
                766.87591553, 616.44366455, 513.61437988, 454.42349243, 357.43081665, 215.41030884,
                150.84393311, 265.34576416, 477.49368286, 609.88610840, 663.50762939, 905.93286133,
                1568.00756836, 2459.11596680, 3014.72949219, 2829.85058594, 2106.92773438,
                1461.13525391, 1251.98889160, 1201.65747070, 759.56903076, -204.43519592,
                -1193.20263672, -1606.24084473, -1362.28906250, -889.43450928, -623.62377930,
                -629.06854248, -739.67553711, -906.91107178, -1238.54333496, -1721.64721680,
                -2107.76074219, -2201.90405273, -2164.79638672, -2335.07397461, -2725.96752930,
                -2877.38964844, -2354.03491211, -1316.94714355, -424.95755005, -140.33351135,
                -275.60760498, -313.49884033, -55.00619125, 217.41998291, 189.98896790,
                -74.05432129, -283.11822510, -342.96221924, -439.78967285, -691.36804199,
                -894.36816406, -755.51501465, -317.44464111, -2.13372803, -169.71307373,
                -706.36108398, -1138.64990234, -1155.49169922, -945.76477051, -972.16461182,
                -1439.88977051, -2055.10034180, -2331.53393555, -2084.40576172, -1559.44702148,
                -1096.94372559, -775.87060547, -455.06170654, -82.31356812, 194.02728271,
                258.22738647, 236.30032349, 340.99520874, 556.61810303, 636.15148926, 455.35351562,
                239.15704346, 323.05108643, 752.74279785, 1249.45446777, 1556.64465332,
                1677.80908203, 1726.92321777, 1689.56518555, 1470.55737305, 1109.83325195,
                776.87097168, 526.15704346, 212.28504944, -255.40719604, -679.69671631,
                -746.10052490, -407.59320068, 36.48087311, 278.80212402, 309.99313354,
                322.37878418, 408.56207275, 479.56372070, 450.52853394, 373.65832520, 348.04833984,
                393.60064697, 482.71276855, 635.98400879, 900.37347412, 1248.46020508,
                1570.72167969, 1772.90026855, 1832.18945312, 1765.37573242, 1610.50292969,
                1448.62683105, 1357.18957520, 1292.01306152, 1092.47106934, 694.12042236,
                284.46432495, 175.74569702, 433.91598511, 794.92321777, 976.06262207, 965.05694580,
                926.51525879, 893.19439697, 743.53436279, 468.06250000, 252.97090149, 215.00091553,
                208.97006226, 67.45935059, -113.67736053, -114.23457336, 98.23759460, 304.27154541,
                299.33395386, 196.58602905, 271.10452271, 510.83450317, 635.06201172, 670.57946777,
                1206.65551758, 2617.97265625, 4151.42773438, 4497.76171875, 3343.18310547,
                1781.97985840, 1066.66601562, 1223.43408203, 1144.42309570, 12.25622559,
                -1721.55346680, -2864.14868164, -2784.53686523, -2009.71923828, -1394.59838867,
                -1117.15991211, -937.78857422, -909.32781982, -1284.25048828, -1966.30151367,
                -2422.94775391, -2272.19604492, -1763.02160645, -1520.19885254,
            ];
            for (i, (c, r)) in izip!(&output[..samples_written], data_out_ref).enumerate() {
                assert!(
                    (c - r).abs() < (2_i32.pow(15) as f32).recip(),
                    "Value {} differs too much: value is {}, ref is {}, deviation is {}.",
                    i,
                    c,
                    r,
                    (c - r).abs()
                );
            }
        }

        assert!(mdct.ov_offset == 336);
    }
}
