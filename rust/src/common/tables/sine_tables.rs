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
//! Sine tables

use num_complex::Complex;

pub fn get_table(length: u16, sin_step: &mut usize) -> Option<&'static [Complex<f32>]> {
    // Get ld2 of length - 2 + 1
    // -2: because first table entry is window of size 4
    // +1: because we already include +1 because of ceil(log2(length))
    let ld2_length = u16::BITS - 1 - length.leading_zeros() - 2 + 1;
    let sin_twiddle_table: Option<&[Complex<f32>]>;

    // Determine table type
    match (length) >> (ld2_length - 1) {
        0x4 =>
        // radix 2
        {
            sin_twiddle_table = Some(&SINE_TABLE_1024);
            *sin_step = 1 << (10 - ld2_length);
        }
        0x7 =>
        // 10 ms
        {
            sin_twiddle_table = Some(&SINE_TABLE_480);
            *sin_step = 1 << (8 - ld2_length);
        }
        0x6 =>
        // 3/4 of radix 2
        {
            sin_twiddle_table = Some(&SINE_TABLE_384);
            *sin_step = 1 << (8 - ld2_length);
        }
        0x5 =>
        // 5/16 of radix 2
        {
            sin_twiddle_table = Some(&SINE_TABLE_80);
            *sin_step = 1 << (6 - ld2_length);
        }
        _ => {
            sin_twiddle_table = None;
            *sin_step = 0;
        }
    }

    assert!(*sin_step > 0);

    sin_twiddle_table
}

#[rustfmt::skip]
pub static SINE_TABLE_32: [Complex<f32>; 17] = [

  Complex { re: 1.000000000, im: 0.000000000 }, Complex { re: 0.998795456, im: 0.049067674 }, Complex { re: 0.995184727, im: 0.098017140 }, Complex { re: 0.989176510, im: 0.146730474 },
  Complex { re: 0.980785280, im: 0.195090322 }, Complex { re: 0.970031253, im: 0.242980180 }, Complex { re: 0.956940336, im: 0.290284677 }, Complex { re: 0.941544065, im: 0.336889853 },
  Complex { re: 0.923879533, im: 0.382683432 }, Complex { re: 0.903989293, im: 0.427555093 }, Complex { re: 0.881921264, im: 0.471396737 }, Complex { re: 0.857728610, im: 0.514102744 },
  Complex { re: 0.831469612, im: 0.555570233 }, Complex { re: 0.803207531, im: 0.595699304 }, Complex { re: 0.773010453, im: 0.634393284 }, Complex { re: 0.740951125, im: 0.671558955 },
  Complex { re: 0.707106781, im: 0.707106781 },
];
#[rustfmt::skip]
pub static SINE_TABLE_48: [Complex<f32>; 25] = [

  Complex { re: 1.000000000, im: 0.000000000 }, Complex { re: 0.999464587, im: 0.032719083 }, Complex { re: 0.997858923, im: 0.065403129 }, Complex { re: 0.995184727, im: 0.098017140 },
  Complex { re: 0.991444861, im: 0.130526192 }, Complex { re: 0.986643332, im: 0.162895473 }, Complex { re: 0.980785280, im: 0.195090322 }, Complex { re: 0.973876979, im: 0.227076263 },
  Complex { re: 0.965925826, im: 0.258819045 }, Complex { re: 0.956940336, im: 0.290284677 }, Complex { re: 0.946930129, im: 0.321439465 }, Complex { re: 0.935905927, im: 0.352250048 },
  Complex { re: 0.923879533, im: 0.382683432 }, Complex { re: 0.910863825, im: 0.412707030 }, Complex { re: 0.896872742, im: 0.442288690 }, Complex { re: 0.881921264, im: 0.471396737 },
  Complex { re: 0.866025404, im: 0.500000000 }, Complex { re: 0.849202182, im: 0.528067851 }, Complex { re: 0.831469612, im: 0.555570233 }, Complex { re: 0.812846685, im: 0.582477697 },
  Complex { re: 0.793353340, im: 0.608761429 }, Complex { re: 0.773010453, im: 0.634393284 }, Complex { re: 0.751839807, im: 0.659345815 }, Complex { re: 0.729864073, im: 0.683592302 },
  Complex { re: 0.707106781, im: 0.707106781 },
];
#[rustfmt::skip]
pub static SINE_TABLE_64: [Complex<f32>; 33] = [

  Complex { re: 1.000000000, im: 0.000000000 }, Complex { re: 0.999698819, im: 0.024541229 }, Complex { re: 0.998795456, im: 0.049067674 }, Complex { re: 0.997290457, im: 0.073564564 },
  Complex { re: 0.995184727, im: 0.098017140 }, Complex { re: 0.992479535, im: 0.122410675 }, Complex { re: 0.989176510, im: 0.146730474 }, Complex { re: 0.985277642, im: 0.170961889 },
  Complex { re: 0.980785280, im: 0.195090322 }, Complex { re: 0.975702130, im: 0.219101240 }, Complex { re: 0.970031253, im: 0.242980180 }, Complex { re: 0.963776066, im: 0.266712757 },
  Complex { re: 0.956940336, im: 0.290284677 }, Complex { re: 0.949528181, im: 0.313681740 }, Complex { re: 0.941544065, im: 0.336889853 }, Complex { re: 0.932992799, im: 0.359895037 },
  Complex { re: 0.923879533, im: 0.382683432 }, Complex { re: 0.914209756, im: 0.405241314 }, Complex { re: 0.903989293, im: 0.427555093 }, Complex { re: 0.893224301, im: 0.449611330 },
  Complex { re: 0.881921264, im: 0.471396737 }, Complex { re: 0.870086991, im: 0.492898192 }, Complex { re: 0.857728610, im: 0.514102744 }, Complex { re: 0.844853565, im: 0.534997620 },
  Complex { re: 0.831469612, im: 0.555570233 }, Complex { re: 0.817584813, im: 0.575808191 }, Complex { re: 0.803207531, im: 0.595699304 }, Complex { re: 0.788346428, im: 0.615231591 },
  Complex { re: 0.773010453, im: 0.634393284 }, Complex { re: 0.757208847, im: 0.653172843 }, Complex { re: 0.740951125, im: 0.671558955 }, Complex { re: 0.724247083, im: 0.689540545 },
  Complex { re: 0.707106781, im: 0.707106781 },
];
#[rustfmt::skip]
pub static SINE_TABLE_80: [Complex<f32>; 41] = [

  Complex { re: 1.000000000, im: 0.000000000 }, Complex { re: 0.999807240, im: 0.019633692 }, Complex { re: 0.999229036, im: 0.039259816 }, Complex { re: 0.998265610, im: 0.058870804 },
  Complex { re: 0.996917334, im: 0.078459096 }, Complex { re: 0.995184727, im: 0.098017140 }, Complex { re: 0.993068457, im: 0.117537397 }, Complex { re: 0.990569340, im: 0.137012342 },
  Complex { re: 0.987688341, im: 0.156434465 }, Complex { re: 0.984426568, im: 0.175796280 }, Complex { re: 0.980785280, im: 0.195090322 }, Complex { re: 0.976765881, im: 0.214309153 },
  Complex { re: 0.972369920, im: 0.233445364 }, Complex { re: 0.967599092, im: 0.252491577 }, Complex { re: 0.962455236, im: 0.271440450 }, Complex { re: 0.956940336, im: 0.290284677 },
  Complex { re: 0.951056516, im: 0.309016994 }, Complex { re: 0.944806046, im: 0.327630180 }, Complex { re: 0.938191336, im: 0.346117057 }, Complex { re: 0.931214935, im: 0.364470500 },
  Complex { re: 0.923879533, im: 0.382683432 }, Complex { re: 0.916187957, im: 0.400748833 }, Complex { re: 0.908143174, im: 0.418659738 }, Complex { re: 0.899748284, im: 0.436409241 },
  Complex { re: 0.891006524, im: 0.453990500 }, Complex { re: 0.881921264, im: 0.471396737 }, Complex { re: 0.872496007, im: 0.488621241 }, Complex { re: 0.862734386, im: 0.505657373 },
  Complex { re: 0.852640164, im: 0.522498565 }, Complex { re: 0.842217234, im: 0.539138323 }, Complex { re: 0.831469612, im: 0.555570233 }, Complex { re: 0.820401444, im: 0.571787960 },
  Complex { re: 0.809016994, im: 0.587785252 }, Complex { re: 0.797320654, im: 0.603555942 }, Complex { re: 0.785316931, im: 0.619093949 }, Complex { re: 0.773010453, im: 0.634393284 },
  Complex { re: 0.760405966, im: 0.649448048 }, Complex { re: 0.747508327, im: 0.664252438 }, Complex { re: 0.734322509, im: 0.678800746 }, Complex { re: 0.720853597, im: 0.693087363 },
  Complex { re: 0.707106781, im: 0.707106781 },
];
#[rustfmt::skip]
pub static SINE_TABLE_384: [Complex<f32>; 193] = [

  Complex { re: 1.000000000, im: 0.000000000 }, Complex { re: 0.999991633, im: 0.004090604 }, Complex { re: 0.999966534, im: 0.008181140 }, Complex { re: 0.999924702, im: 0.012271538 },
  Complex { re: 0.999866138, im: 0.016361732 }, Complex { re: 0.999790843, im: 0.020451651 }, Complex { re: 0.999698819, im: 0.024541229 }, Complex { re: 0.999590066, im: 0.028630395 },
  Complex { re: 0.999464587, im: 0.032719083 }, Complex { re: 0.999322385, im: 0.036807223 }, Complex { re: 0.999163460, im: 0.040894747 }, Complex { re: 0.998987816, im: 0.044981587 },
  Complex { re: 0.998795456, im: 0.049067674 }, Complex { re: 0.998586383, im: 0.053152941 }, Complex { re: 0.998360601, im: 0.057237317 }, Complex { re: 0.998118113, im: 0.061320736 },
  Complex { re: 0.997858923, im: 0.065403129 }, Complex { re: 0.997583036, im: 0.069484428 }, Complex { re: 0.997290457, im: 0.073564564 }, Complex { re: 0.996981189, im: 0.077643468 },
  Complex { re: 0.996655239, im: 0.081721074 }, Complex { re: 0.996312612, im: 0.085797312 }, Complex { re: 0.995953314, im: 0.089872115 }, Complex { re: 0.995577350, im: 0.093945414 },
  Complex { re: 0.995184727, im: 0.098017140 }, Complex { re: 0.994775451, im: 0.102087227 }, Complex { re: 0.994349530, im: 0.106155605 }, Complex { re: 0.993906970, im: 0.110222207 },
  Complex { re: 0.993447779, im: 0.114286965 }, Complex { re: 0.992971965, im: 0.118349810 }, Complex { re: 0.992479535, im: 0.122410675 }, Complex { re: 0.991970497, im: 0.126469492 },
  Complex { re: 0.991444861, im: 0.130526192 }, Complex { re: 0.990902635, im: 0.134580709 }, Complex { re: 0.990343829, im: 0.138632973 }, Complex { re: 0.989768450, im: 0.142682917 },
  Complex { re: 0.989176510, im: 0.146730474 }, Complex { re: 0.988568018, im: 0.150775576 }, Complex { re: 0.987942984, im: 0.154818155 }, Complex { re: 0.987301418, im: 0.158858143 },
  Complex { re: 0.986643332, im: 0.162895473 }, Complex { re: 0.985968736, im: 0.166930078 }, Complex { re: 0.985277642, im: 0.170961889 }, Complex { re: 0.984570062, im: 0.174990839 },
  Complex { re: 0.983846006, im: 0.179016861 }, Complex { re: 0.983105487, im: 0.183039888 }, Complex { re: 0.982348519, im: 0.187059852 }, Complex { re: 0.981575112, im: 0.191076686 },
  Complex { re: 0.980785280, im: 0.195090322 }, Complex { re: 0.979979037, im: 0.199100694 }, Complex { re: 0.979156396, im: 0.203107734 }, Complex { re: 0.978317371, im: 0.207111376 },
  Complex { re: 0.977461975, im: 0.211111552 }, Complex { re: 0.976590223, im: 0.215108196 }, Complex { re: 0.975702130, im: 0.219101240 }, Complex { re: 0.974797710, im: 0.223090618 },
  Complex { re: 0.973876979, im: 0.227076263 }, Complex { re: 0.972939952, im: 0.231058108 }, Complex { re: 0.971986645, im: 0.235036087 }, Complex { re: 0.971017073, im: 0.239010133 },
  Complex { re: 0.970031253, im: 0.242980180 }, Complex { re: 0.969029202, im: 0.246946161 }, Complex { re: 0.968010935, im: 0.250908009 }, Complex { re: 0.966976471, im: 0.254865660 },
  Complex { re: 0.965925826, im: 0.258819045 }, Complex { re: 0.964859019, im: 0.262768100 }, Complex { re: 0.963776066, im: 0.266712757 }, Complex { re: 0.962676986, im: 0.270652952 },
  Complex { re: 0.961561798, im: 0.274588618 }, Complex { re: 0.960430519, im: 0.278519689 }, Complex { re: 0.959283170, im: 0.282446100 }, Complex { re: 0.958119769, im: 0.286367785 },
  Complex { re: 0.956940336, im: 0.290284677 }, Complex { re: 0.955744890, im: 0.294196713 }, Complex { re: 0.954533451, im: 0.298103825 }, Complex { re: 0.953306040, im: 0.302005949 },
  Complex { re: 0.952062678, im: 0.305903020 }, Complex { re: 0.950803384, im: 0.309794972 }, Complex { re: 0.949528181, im: 0.313681740 }, Complex { re: 0.948237089, im: 0.317563260 },
  Complex { re: 0.946930129, im: 0.321439465 }, Complex { re: 0.945607325, im: 0.325310292 }, Complex { re: 0.944268698, im: 0.329175676 }, Complex { re: 0.942914271, im: 0.333035551 },
  Complex { re: 0.941544065, im: 0.336889853 }, Complex { re: 0.940158105, im: 0.340738519 }, Complex { re: 0.938756412, im: 0.344581482 }, Complex { re: 0.937339012, im: 0.348418680 },
  Complex { re: 0.935905927, im: 0.352250048 }, Complex { re: 0.934457181, im: 0.356075521 }, Complex { re: 0.932992799, im: 0.359895037 }, Complex { re: 0.931512805, im: 0.363708530 },
  Complex { re: 0.930017224, im: 0.367515937 }, Complex { re: 0.928506080, im: 0.371317194 }, Complex { re: 0.926979400, im: 0.375112238 }, Complex { re: 0.925437209, im: 0.378901005 },
  Complex { re: 0.923879533, im: 0.382683432 }, Complex { re: 0.922306396, im: 0.386459456 }, Complex { re: 0.920717827, im: 0.390229013 }, Complex { re: 0.919113852, im: 0.393992040 },
  Complex { re: 0.917494496, im: 0.397748475 }, Complex { re: 0.915859789, im: 0.401498253 }, Complex { re: 0.914209756, im: 0.405241314 }, Complex { re: 0.912544425, im: 0.408977594 },
  Complex { re: 0.910863825, im: 0.412707030 }, Complex { re: 0.909167983, im: 0.416429560 }, Complex { re: 0.907456928, im: 0.420145122 }, Complex { re: 0.905730688, im: 0.423853654 },
  Complex { re: 0.903989293, im: 0.427555093 }, Complex { re: 0.902232771, im: 0.431249379 }, Complex { re: 0.900461152, im: 0.434936447 }, Complex { re: 0.898674466, im: 0.438616239 },
  Complex { re: 0.896872742, im: 0.442288690 }, Complex { re: 0.895056010, im: 0.445953741 }, Complex { re: 0.893224301, im: 0.449611330 }, Complex { re: 0.891377646, im: 0.453261395 },
  Complex { re: 0.889516075, im: 0.456903876 }, Complex { re: 0.887639620, im: 0.460538711 }, Complex { re: 0.885748312, im: 0.464165840 }, Complex { re: 0.883842183, im: 0.467785202 },
  Complex { re: 0.881921264, im: 0.471396737 }, Complex { re: 0.879985588, im: 0.475000384 }, Complex { re: 0.878035187, im: 0.478596082 }, Complex { re: 0.876070094, im: 0.482183772 },
  Complex { re: 0.874090342, im: 0.485763394 }, Complex { re: 0.872095963, im: 0.489334887 }, Complex { re: 0.870086991, im: 0.492898192 }, Complex { re: 0.868063460, im: 0.496453250 },
  Complex { re: 0.866025404, im: 0.500000000 }, Complex { re: 0.863972856, im: 0.503538384 }, Complex { re: 0.861905852, im: 0.507068342 }, Complex { re: 0.859824425, im: 0.510589815 },
  Complex { re: 0.857728610, im: 0.514102744 }, Complex { re: 0.855618443, im: 0.517607071 }, Complex { re: 0.853493959, im: 0.521102737 }, Complex { re: 0.851355193, im: 0.524589683 },
  Complex { re: 0.849202182, im: 0.528067851 }, Complex { re: 0.847034960, im: 0.531537182 }, Complex { re: 0.844853565, im: 0.534997620 }, Complex { re: 0.842658033, im: 0.538449105 },
  Complex { re: 0.840448401, im: 0.541891581 }, Complex { re: 0.838224706, im: 0.545324988 }, Complex { re: 0.835986984, im: 0.548749271 }, Complex { re: 0.833735274, im: 0.552164372 },
  Complex { re: 0.831469612, im: 0.555570233 }, Complex { re: 0.829190038, im: 0.558966798 }, Complex { re: 0.826896589, im: 0.562354009 }, Complex { re: 0.824589303, im: 0.565731811 },
  Complex { re: 0.822268219, im: 0.569100146 }, Complex { re: 0.819933376, im: 0.572458958 }, Complex { re: 0.817584813, im: 0.575808191 }, Complex { re: 0.815222569, im: 0.579147790 },
  Complex { re: 0.812846685, im: 0.582477697 }, Complex { re: 0.810457198, im: 0.585797857 }, Complex { re: 0.808054150, im: 0.589108216 }, Complex { re: 0.805637581, im: 0.592408717 },
  Complex { re: 0.803207531, im: 0.595699304 }, Complex { re: 0.800764041, im: 0.598979925 }, Complex { re: 0.798307152, im: 0.602250522 }, Complex { re: 0.795836905, im: 0.605511041 },
  Complex { re: 0.793353340, im: 0.608761429 }, Complex { re: 0.790856501, im: 0.612001630 }, Complex { re: 0.788346428, im: 0.615231591 }, Complex { re: 0.785823163, im: 0.618451256 },
  Complex { re: 0.783286749, im: 0.621660573 }, Complex { re: 0.780737229, im: 0.624859488 }, Complex { re: 0.778174644, im: 0.628047947 }, Complex { re: 0.775599038, im: 0.631225897 },
  Complex { re: 0.773010453, im: 0.634393284 }, Complex { re: 0.770408934, im: 0.637550056 }, Complex { re: 0.767794524, im: 0.640696160 }, Complex { re: 0.765167266, im: 0.643831543 },
  Complex { re: 0.762527204, im: 0.646956153 }, Complex { re: 0.759874383, im: 0.650069937 }, Complex { re: 0.757208847, im: 0.653172843 }, Complex { re: 0.754530640, im: 0.656264820 },
  Complex { re: 0.751839807, im: 0.659345815 }, Complex { re: 0.749136395, im: 0.662415778 }, Complex { re: 0.746420446, im: 0.665474656 }, Complex { re: 0.743692008, im: 0.668522399 },
  Complex { re: 0.740951125, im: 0.671558955 }, Complex { re: 0.738197844, im: 0.674584274 }, Complex { re: 0.735432211, im: 0.677598305 }, Complex { re: 0.732654272, im: 0.680600998 },
  Complex { re: 0.729864073, im: 0.683592302 }, Complex { re: 0.727061661, im: 0.686572168 }, Complex { re: 0.724247083, im: 0.689540545 }, Complex { re: 0.721420386, im: 0.692497384 },
  Complex { re: 0.718581618, im: 0.695442635 }, Complex { re: 0.715730825, im: 0.698376249 }, Complex { re: 0.712868056, im: 0.701298178 }, Complex { re: 0.709993359, im: 0.704208371 },
  Complex { re: 0.707106781, im: 0.707106781 },
];
#[rustfmt::skip]
pub static SINE_TABLE_480: [Complex<f32>; 241] = [

  Complex { re: 1.000000000, im: 0.000000000 }, Complex { re: 0.999994645, im: 0.003272487 }, Complex { re: 0.999978582, im: 0.006544938 }, Complex { re: 0.999951809, im: 0.009817319 },
  Complex { re: 0.999914328, im: 0.013089596 }, Complex { re: 0.999866138, im: 0.016361732 }, Complex { re: 0.999807240, im: 0.019633692 }, Complex { re: 0.999737636, im: 0.022905443 },
  Complex { re: 0.999657325, im: 0.026176948 }, Complex { re: 0.999566309, im: 0.029448173 }, Complex { re: 0.999464587, im: 0.032719083 }, Complex { re: 0.999352163, im: 0.035989642 },
  Complex { re: 0.999229036, im: 0.039259816 }, Complex { re: 0.999095209, im: 0.042529569 }, Complex { re: 0.998950681, im: 0.045798867 }, Complex { re: 0.998795456, im: 0.049067674 },
  Complex { re: 0.998629535, im: 0.052335956 }, Complex { re: 0.998452919, im: 0.055603678 }, Complex { re: 0.998265610, im: 0.058870804 }, Complex { re: 0.998067611, im: 0.062137299 },
  Complex { re: 0.997858923, im: 0.065403129 }, Complex { re: 0.997639549, im: 0.068668259 }, Complex { re: 0.997409491, im: 0.071932653 }, Complex { re: 0.997168752, im: 0.075196277 },
  Complex { re: 0.996917334, im: 0.078459096 }, Complex { re: 0.996655239, im: 0.081721074 }, Complex { re: 0.996382472, im: 0.084982177 }, Complex { re: 0.996099033, im: 0.088242371 },
  Complex { re: 0.995804928, im: 0.091501619 }, Complex { re: 0.995500158, im: 0.094759887 }, Complex { re: 0.995184727, im: 0.098017140 }, Complex { re: 0.994858638, im: 0.101273344 },
  Complex { re: 0.994521895, im: 0.104528463 }, Complex { re: 0.994174502, im: 0.107782463 }, Complex { re: 0.993816462, im: 0.111035309 }, Complex { re: 0.993447779, im: 0.114286965 },
  Complex { re: 0.993068457, im: 0.117537397 }, Complex { re: 0.992678500, im: 0.120786571 }, Complex { re: 0.992277912, im: 0.124034451 }, Complex { re: 0.991866698, im: 0.127281003 },
  Complex { re: 0.991444861, im: 0.130526192 }, Complex { re: 0.991012407, im: 0.133769983 }, Complex { re: 0.990569340, im: 0.137012342 }, Complex { re: 0.990115665, im: 0.140253233 },
  Complex { re: 0.989651387, im: 0.143492622 }, Complex { re: 0.989176510, im: 0.146730474 }, Complex { re: 0.988691040, im: 0.149966756 }, Complex { re: 0.988194982, im: 0.153201431 },
  Complex { re: 0.987688341, im: 0.156434465 }, Complex { re: 0.987171122, im: 0.159665824 }, Complex { re: 0.986643332, im: 0.162895473 }, Complex { re: 0.986104976, im: 0.166123378 },
  Complex { re: 0.985556059, im: 0.169349504 }, Complex { re: 0.984996588, im: 0.172573816 }, Complex { re: 0.984426568, im: 0.175796280 }, Complex { re: 0.983846006, im: 0.179016861 },
  Complex { re: 0.983254908, im: 0.182235525 }, Complex { re: 0.982653279, im: 0.185452238 }, Complex { re: 0.982041128, im: 0.188666965 }, Complex { re: 0.981418459, im: 0.191879671 },
  Complex { re: 0.980785280, im: 0.195090322 }, Complex { re: 0.980141598, im: 0.198298884 }, Complex { re: 0.979487420, im: 0.201505322 }, Complex { re: 0.978822751, im: 0.204709603 },
  Complex { re: 0.978147601, im: 0.207911691 }, Complex { re: 0.977461975, im: 0.211111552 }, Complex { re: 0.976765881, im: 0.214309153 }, Complex { re: 0.976059327, im: 0.217504459 },
  Complex { re: 0.975342321, im: 0.220697435 }, Complex { re: 0.974614869, im: 0.223888048 }, Complex { re: 0.973876979, im: 0.227076263 }, Complex { re: 0.973128661, im: 0.230262046 },
  Complex { re: 0.972369920, im: 0.233445364 }, Complex { re: 0.971600767, im: 0.236626181 }, Complex { re: 0.970821208, im: 0.239804465 }, Complex { re: 0.970031253, im: 0.242980180 },
  Complex { re: 0.969230910, im: 0.246153293 }, Complex { re: 0.968420187, im: 0.249323770 }, Complex { re: 0.967599092, im: 0.252491577 }, Complex { re: 0.966767636, im: 0.255656680 },
  Complex { re: 0.965925826, im: 0.258819045 }, Complex { re: 0.965073672, im: 0.261978638 }, Complex { re: 0.964211183, im: 0.265135426 }, Complex { re: 0.963338368, im: 0.268289375 },
  Complex { re: 0.962455236, im: 0.271440450 }, Complex { re: 0.961561798, im: 0.274588618 }, Complex { re: 0.960658061, im: 0.277733846 }, Complex { re: 0.959744037, im: 0.280876099 },
  Complex { re: 0.958819735, im: 0.284015345 }, Complex { re: 0.957885164, im: 0.287151549 }, Complex { re: 0.956940336, im: 0.290284677 }, Complex { re: 0.955985259, im: 0.293414697 },
  Complex { re: 0.955019944, im: 0.296541575 }, Complex { re: 0.954044402, im: 0.299665277 }, Complex { re: 0.953058643, im: 0.302785770 }, Complex { re: 0.952062678, im: 0.305903020 },
  Complex { re: 0.951056516, im: 0.309016994 }, Complex { re: 0.950040170, im: 0.312127659 }, Complex { re: 0.949013649, im: 0.315234982 }, Complex { re: 0.947976965, im: 0.318338928 },
  Complex { re: 0.946930129, im: 0.321439465 }, Complex { re: 0.945873153, im: 0.324536560 }, Complex { re: 0.944806046, im: 0.327630180 }, Complex { re: 0.943728822, im: 0.330720290 },
  Complex { re: 0.942641491, im: 0.333806859 }, Complex { re: 0.941544065, im: 0.336889853 }, Complex { re: 0.940436556, im: 0.339969240 }, Complex { re: 0.939318976, im: 0.343044985 },
  Complex { re: 0.938191336, im: 0.346117057 }, Complex { re: 0.937053649, im: 0.349185422 }, Complex { re: 0.935905927, im: 0.352250048 }, Complex { re: 0.934748182, im: 0.355310901 },
  Complex { re: 0.933580426, im: 0.358367950 }, Complex { re: 0.932402673, im: 0.361421160 }, Complex { re: 0.931214935, im: 0.364470500 }, Complex { re: 0.930017224, im: 0.367515937 },
  Complex { re: 0.928809553, im: 0.370557438 }, Complex { re: 0.927591935, im: 0.373594970 }, Complex { re: 0.926364384, im: 0.376628502 }, Complex { re: 0.925126912, im: 0.379658000 },
  Complex { re: 0.923879533, im: 0.382683432 }, Complex { re: 0.922622259, im: 0.385704767 }, Complex { re: 0.921355105, im: 0.388721970 }, Complex { re: 0.920078084, im: 0.391735011 },
  Complex { re: 0.918791210, im: 0.394743856 }, Complex { re: 0.917494496, im: 0.397748475 }, Complex { re: 0.916187957, im: 0.400748833 }, Complex { re: 0.914871606, im: 0.403744900 },
  Complex { re: 0.913545458, im: 0.406736643 }, Complex { re: 0.912209526, im: 0.409724030 }, Complex { re: 0.910863825, im: 0.412707030 }, Complex { re: 0.909508369, im: 0.415685610 },
  Complex { re: 0.908143174, im: 0.418659738 }, Complex { re: 0.906768253, im: 0.421629382 }, Complex { re: 0.905383621, im: 0.424594511 }, Complex { re: 0.903989293, im: 0.427555093 },
  Complex { re: 0.902585284, im: 0.430511097 }, Complex { re: 0.901171610, im: 0.433462490 }, Complex { re: 0.899748284, im: 0.436409241 }, Complex { re: 0.898315323, im: 0.439351318 },
  Complex { re: 0.896872742, im: 0.442288690 }, Complex { re: 0.895420555, im: 0.445221326 }, Complex { re: 0.893958780, im: 0.448149194 }, Complex { re: 0.892487431, im: 0.451072262 },
  Complex { re: 0.891006524, im: 0.453990500 }, Complex { re: 0.889516075, im: 0.456903876 }, Complex { re: 0.888016101, im: 0.459812358 }, Complex { re: 0.886506616, im: 0.462715917 },
  Complex { re: 0.884987637, im: 0.465614520 }, Complex { re: 0.883459181, im: 0.468508137 }, Complex { re: 0.881921264, im: 0.471396737 }, Complex { re: 0.880373903, im: 0.474280288 },
  Complex { re: 0.878817113, im: 0.477158760 }, Complex { re: 0.877250911, im: 0.480032122 }, Complex { re: 0.875675315, im: 0.482900344 }, Complex { re: 0.874090342, im: 0.485763394 },
  Complex { re: 0.872496007, im: 0.488621241 }, Complex { re: 0.870892329, im: 0.491473857 }, Complex { re: 0.869279324, im: 0.494321208 }, Complex { re: 0.867657010, im: 0.497163266 },
  Complex { re: 0.866025404, im: 0.500000000 }, Complex { re: 0.864384523, im: 0.502831379 }, Complex { re: 0.862734386, im: 0.505657373 }, Complex { re: 0.861075009, im: 0.508477952 },
  Complex { re: 0.859406412, im: 0.511293086 }, Complex { re: 0.857728610, im: 0.514102744 }, Complex { re: 0.856041623, im: 0.516906897 }, Complex { re: 0.854345468, im: 0.519705514 },
  Complex { re: 0.852640164, im: 0.522498565 }, Complex { re: 0.850925729, im: 0.525286020 }, Complex { re: 0.849202182, im: 0.528067851 }, Complex { re: 0.847469539, im: 0.530844026 },
  Complex { re: 0.845727822, im: 0.533614516 }, Complex { re: 0.843977047, im: 0.536379292 }, Complex { re: 0.842217234, im: 0.539138323 }, Complex { re: 0.840448401, im: 0.541891581 },
  Complex { re: 0.838670568, im: 0.544639035 }, Complex { re: 0.836883753, im: 0.547380657 }, Complex { re: 0.835087976, im: 0.550116417 }, Complex { re: 0.833283256, im: 0.552846285 },
  Complex { re: 0.831469612, im: 0.555570233 }, Complex { re: 0.829647064, im: 0.558288231 }, Complex { re: 0.827815631, im: 0.561000251 }, Complex { re: 0.825975333, im: 0.563706262 },
  Complex { re: 0.824126189, im: 0.566406237 }, Complex { re: 0.822268219, im: 0.569100146 }, Complex { re: 0.820401444, im: 0.571787960 }, Complex { re: 0.818525882, im: 0.574469651 },
  Complex { re: 0.816641555, im: 0.577145190 }, Complex { re: 0.814748483, im: 0.579814548 }, Complex { re: 0.812846685, im: 0.582477697 }, Complex { re: 0.810936182, im: 0.585134608 },
  Complex { re: 0.809016994, im: 0.587785252 }, Complex { re: 0.807089143, im: 0.590429602 }, Complex { re: 0.805152649, im: 0.593067629 }, Complex { re: 0.803207531, im: 0.595699304 },
  Complex { re: 0.801253813, im: 0.598324601 }, Complex { re: 0.799291513, im: 0.600943489 }, Complex { re: 0.797320654, im: 0.603555942 }, Complex { re: 0.795341256, im: 0.606161931 },
  Complex { re: 0.793353340, im: 0.608761429 }, Complex { re: 0.791356929, im: 0.611354407 }, Complex { re: 0.789352042, im: 0.613940839 }, Complex { re: 0.787338702, im: 0.616520695 },
  Complex { re: 0.785316931, im: 0.619093949 }, Complex { re: 0.783286749, im: 0.621660573 }, Complex { re: 0.781248179, im: 0.624220540 }, Complex { re: 0.779201243, im: 0.626773822 },
  Complex { re: 0.777145961, im: 0.629320391 }, Complex { re: 0.775082358, im: 0.631860221 }, Complex { re: 0.773010453, im: 0.634393284 }, Complex { re: 0.770930271, im: 0.636919554 },
  Complex { re: 0.768841832, im: 0.639439002 }, Complex { re: 0.766745160, im: 0.641951603 }, Complex { re: 0.764640276, im: 0.644457328 }, Complex { re: 0.762527204, im: 0.646956153 },
  Complex { re: 0.760405966, im: 0.649448048 }, Complex { re: 0.758276584, im: 0.651932989 }, Complex { re: 0.756139082, im: 0.654410948 }, Complex { re: 0.753993482, im: 0.656881899 },
  Complex { re: 0.751839807, im: 0.659345815 }, Complex { re: 0.749678081, im: 0.661802670 }, Complex { re: 0.747508327, im: 0.664252438 }, Complex { re: 0.745330567, im: 0.666695092 },
  Complex { re: 0.743144825, im: 0.669130606 }, Complex { re: 0.740951125, im: 0.671558955 }, Complex { re: 0.738749490, im: 0.673980111 }, Complex { re: 0.736539944, im: 0.676394050 },
  Complex { re: 0.734322509, im: 0.678800746 }, Complex { re: 0.732097211, im: 0.681200171 }, Complex { re: 0.729864073, im: 0.683592302 }, Complex { re: 0.727623118, im: 0.685977112 },
  Complex { re: 0.725374371, im: 0.688354576 }, Complex { re: 0.723117856, im: 0.690724668 }, Complex { re: 0.720853597, im: 0.693087363 }, Complex { re: 0.718581618, im: 0.695442635 },
  Complex { re: 0.716301943, im: 0.697790460 }, Complex { re: 0.714014598, im: 0.700130812 }, Complex { re: 0.711719606, im: 0.702463666 }, Complex { re: 0.709416992, im: 0.704788998 },
  Complex { re: 0.707106781, im: 0.707106781 },
];
#[rustfmt::skip]
pub static SINE_TABLE_512: [Complex<f32>; 257] = [

  Complex { re: 1.000000000, im: 0.000000000 }, Complex { re: 0.999995294, im: 0.003067957 }, Complex { re: 0.999981175, im: 0.006135885 }, Complex { re: 0.999957645, im: 0.009203755 },
  Complex { re: 0.999924702, im: 0.012271538 }, Complex { re: 0.999882347, im: 0.015339206 }, Complex { re: 0.999830582, im: 0.018406730 }, Complex { re: 0.999769405, im: 0.021474080 },
  Complex { re: 0.999698819, im: 0.024541229 }, Complex { re: 0.999618822, im: 0.027608146 }, Complex { re: 0.999529418, im: 0.030674803 }, Complex { re: 0.999430605, im: 0.033741172 },
  Complex { re: 0.999322385, im: 0.036807223 }, Complex { re: 0.999204759, im: 0.039872928 }, Complex { re: 0.999077728, im: 0.042938257 }, Complex { re: 0.998941293, im: 0.046003182 },
  Complex { re: 0.998795456, im: 0.049067674 }, Complex { re: 0.998640218, im: 0.052131705 }, Complex { re: 0.998475581, im: 0.055195244 }, Complex { re: 0.998301545, im: 0.058258265 },
  Complex { re: 0.998118113, im: 0.061320736 }, Complex { re: 0.997925286, im: 0.064382631 }, Complex { re: 0.997723067, im: 0.067443920 }, Complex { re: 0.997511456, im: 0.070504573 },
  Complex { re: 0.997290457, im: 0.073564564 }, Complex { re: 0.997060070, im: 0.076623861 }, Complex { re: 0.996820299, im: 0.079682438 }, Complex { re: 0.996571146, im: 0.082740265 },
  Complex { re: 0.996312612, im: 0.085797312 }, Complex { re: 0.996044701, im: 0.088853553 }, Complex { re: 0.995767414, im: 0.091908956 }, Complex { re: 0.995480755, im: 0.094963495 },
  Complex { re: 0.995184727, im: 0.098017140 }, Complex { re: 0.994879331, im: 0.101069863 }, Complex { re: 0.994564571, im: 0.104121634 }, Complex { re: 0.994240449, im: 0.107172425 },
  Complex { re: 0.993906970, im: 0.110222207 }, Complex { re: 0.993564136, im: 0.113270952 }, Complex { re: 0.993211949, im: 0.116318631 }, Complex { re: 0.992850414, im: 0.119365215 },
  Complex { re: 0.992479535, im: 0.122410675 }, Complex { re: 0.992099313, im: 0.125454983 }, Complex { re: 0.991709754, im: 0.128498111 }, Complex { re: 0.991310860, im: 0.131540029 },
  Complex { re: 0.990902635, im: 0.134580709 }, Complex { re: 0.990485084, im: 0.137620122 }, Complex { re: 0.990058210, im: 0.140658239 }, Complex { re: 0.989622017, im: 0.143695033 },
  Complex { re: 0.989176510, im: 0.146730474 }, Complex { re: 0.988721692, im: 0.149764535 }, Complex { re: 0.988257568, im: 0.152797185 }, Complex { re: 0.987784142, im: 0.155828398 },
  Complex { re: 0.987301418, im: 0.158858143 }, Complex { re: 0.986809402, im: 0.161886394 }, Complex { re: 0.986308097, im: 0.164913120 }, Complex { re: 0.985797509, im: 0.167938295 },
  Complex { re: 0.985277642, im: 0.170961889 }, Complex { re: 0.984748502, im: 0.173983873 }, Complex { re: 0.984210092, im: 0.177004220 }, Complex { re: 0.983662419, im: 0.180022901 },
  Complex { re: 0.983105487, im: 0.183039888 }, Complex { re: 0.982539302, im: 0.186055152 }, Complex { re: 0.981963869, im: 0.189068664 }, Complex { re: 0.981379193, im: 0.192080397 },
  Complex { re: 0.980785280, im: 0.195090322 }, Complex { re: 0.980182136, im: 0.198098411 }, Complex { re: 0.979569766, im: 0.201104635 }, Complex { re: 0.978948175, im: 0.204108966 },
  Complex { re: 0.978317371, im: 0.207111376 }, Complex { re: 0.977677358, im: 0.210111837 }, Complex { re: 0.977028143, im: 0.213110320 }, Complex { re: 0.976369731, im: 0.216106797 },
  Complex { re: 0.975702130, im: 0.219101240 }, Complex { re: 0.975025345, im: 0.222093621 }, Complex { re: 0.974339383, im: 0.225083911 }, Complex { re: 0.973644250, im: 0.228072083 },
  Complex { re: 0.972939952, im: 0.231058108 }, Complex { re: 0.972226497, im: 0.234041959 }, Complex { re: 0.971503891, im: 0.237023606 }, Complex { re: 0.970772141, im: 0.240003022 },
  Complex { re: 0.970031253, im: 0.242980180 }, Complex { re: 0.969281235, im: 0.245955050 }, Complex { re: 0.968522094, im: 0.248927606 }, Complex { re: 0.967753837, im: 0.251897818 },
  Complex { re: 0.966976471, im: 0.254865660 }, Complex { re: 0.966190003, im: 0.257831102 }, Complex { re: 0.965394442, im: 0.260794118 }, Complex { re: 0.964589793, im: 0.263754679 },
  Complex { re: 0.963776066, im: 0.266712757 }, Complex { re: 0.962953267, im: 0.269668326 }, Complex { re: 0.962121404, im: 0.272621355 }, Complex { re: 0.961280486, im: 0.275571819 },
  Complex { re: 0.960430519, im: 0.278519689 }, Complex { re: 0.959571513, im: 0.281464938 }, Complex { re: 0.958703475, im: 0.284407537 }, Complex { re: 0.957826413, im: 0.287347460 },
  Complex { re: 0.956940336, im: 0.290284677 }, Complex { re: 0.956045251, im: 0.293219163 }, Complex { re: 0.955141168, im: 0.296150888 }, Complex { re: 0.954228095, im: 0.299079826 },
  Complex { re: 0.953306040, im: 0.302005949 }, Complex { re: 0.952375013, im: 0.304929230 }, Complex { re: 0.951435021, im: 0.307849640 }, Complex { re: 0.950486074, im: 0.310767153 },
  Complex { re: 0.949528181, im: 0.313681740 }, Complex { re: 0.948561350, im: 0.316593376 }, Complex { re: 0.947585591, im: 0.319502031 }, Complex { re: 0.946600913, im: 0.322407679 },
  Complex { re: 0.945607325, im: 0.325310292 }, Complex { re: 0.944604837, im: 0.328209844 }, Complex { re: 0.943593458, im: 0.331106306 }, Complex { re: 0.942573198, im: 0.333999651 },
  Complex { re: 0.941544065, im: 0.336889853 }, Complex { re: 0.940506071, im: 0.339776884 }, Complex { re: 0.939459224, im: 0.342660717 }, Complex { re: 0.938403534, im: 0.345541325 },
  Complex { re: 0.937339012, im: 0.348418680 }, Complex { re: 0.936265667, im: 0.351292756 }, Complex { re: 0.935183510, im: 0.354163525 }, Complex { re: 0.934092550, im: 0.357030961 },
  Complex { re: 0.932992799, im: 0.359895037 }, Complex { re: 0.931884266, im: 0.362755724 }, Complex { re: 0.930766961, im: 0.365612998 }, Complex { re: 0.929640896, im: 0.368466830 },
  Complex { re: 0.928506080, im: 0.371317194 }, Complex { re: 0.927362526, im: 0.374164063 }, Complex { re: 0.926210242, im: 0.377007410 }, Complex { re: 0.925049241, im: 0.379847209 },
  Complex { re: 0.923879533, im: 0.382683432 }, Complex { re: 0.922701128, im: 0.385516054 }, Complex { re: 0.921514039, im: 0.388345047 }, Complex { re: 0.920318277, im: 0.391170384 },
  Complex { re: 0.919113852, im: 0.393992040 }, Complex { re: 0.917900776, im: 0.396809987 }, Complex { re: 0.916679060, im: 0.399624200 }, Complex { re: 0.915448716, im: 0.402434651 },
  Complex { re: 0.914209756, im: 0.405241314 }, Complex { re: 0.912962190, im: 0.408044163 }, Complex { re: 0.911706032, im: 0.410843171 }, Complex { re: 0.910441292, im: 0.413638312 },
  Complex { re: 0.909167983, im: 0.416429560 }, Complex { re: 0.907886116, im: 0.419216888 }, Complex { re: 0.906595705, im: 0.422000271 }, Complex { re: 0.905296759, im: 0.424779681 },
  Complex { re: 0.903989293, im: 0.427555093 }, Complex { re: 0.902673318, im: 0.430326481 }, Complex { re: 0.901348847, im: 0.433093819 }, Complex { re: 0.900015892, im: 0.435857080 },
  Complex { re: 0.898674466, im: 0.438616239 }, Complex { re: 0.897324581, im: 0.441371269 }, Complex { re: 0.895966250, im: 0.444122145 }, Complex { re: 0.894599486, im: 0.446868840 },
  Complex { re: 0.893224301, im: 0.449611330 }, Complex { re: 0.891840709, im: 0.452349587 }, Complex { re: 0.890448723, im: 0.455083587 }, Complex { re: 0.889048356, im: 0.457813304 },
  Complex { re: 0.887639620, im: 0.460538711 }, Complex { re: 0.886222530, im: 0.463259784 }, Complex { re: 0.884797098, im: 0.465976496 }, Complex { re: 0.883363339, im: 0.468688822 },
  Complex { re: 0.881921264, im: 0.471396737 }, Complex { re: 0.880470889, im: 0.474100215 }, Complex { re: 0.879012226, im: 0.476799230 }, Complex { re: 0.877545290, im: 0.479493758 },
  Complex { re: 0.876070094, im: 0.482183772 }, Complex { re: 0.874586652, im: 0.484869248 }, Complex { re: 0.873094978, im: 0.487550160 }, Complex { re: 0.871595087, im: 0.490226483 },
  Complex { re: 0.870086991, im: 0.492898192 }, Complex { re: 0.868570706, im: 0.495565262 }, Complex { re: 0.867046246, im: 0.498227667 }, Complex { re: 0.865513624, im: 0.500885383 },
  Complex { re: 0.863972856, im: 0.503538384 }, Complex { re: 0.862423956, im: 0.506186645 }, Complex { re: 0.860866939, im: 0.508830143 }, Complex { re: 0.859301818, im: 0.511468850 },
  Complex { re: 0.857728610, im: 0.514102744 }, Complex { re: 0.856147328, im: 0.516731799 }, Complex { re: 0.854557988, im: 0.519355990 }, Complex { re: 0.852960605, im: 0.521975293 },
  Complex { re: 0.851355193, im: 0.524589683 }, Complex { re: 0.849741768, im: 0.527199135 }, Complex { re: 0.848120345, im: 0.529803625 }, Complex { re: 0.846490939, im: 0.532403128 },
  Complex { re: 0.844853565, im: 0.534997620 }, Complex { re: 0.843208240, im: 0.537587076 }, Complex { re: 0.841554977, im: 0.540171473 }, Complex { re: 0.839893794, im: 0.542750785 },
  Complex { re: 0.838224706, im: 0.545324988 }, Complex { re: 0.836547727, im: 0.547894059 }, Complex { re: 0.834862875, im: 0.550457973 }, Complex { re: 0.833170165, im: 0.553016706 },
  Complex { re: 0.831469612, im: 0.555570233 }, Complex { re: 0.829761234, im: 0.558118531 }, Complex { re: 0.828045045, im: 0.560661576 }, Complex { re: 0.826321063, im: 0.563199344 },
  Complex { re: 0.824589303, im: 0.565731811 }, Complex { re: 0.822849781, im: 0.568258953 }, Complex { re: 0.821102515, im: 0.570780746 }, Complex { re: 0.819347520, im: 0.573297167 },
  Complex { re: 0.817584813, im: 0.575808191 }, Complex { re: 0.815814411, im: 0.578313796 }, Complex { re: 0.814036330, im: 0.580813958 }, Complex { re: 0.812250587, im: 0.583308653 },
  Complex { re: 0.810457198, im: 0.585797857 }, Complex { re: 0.808656182, im: 0.588281548 }, Complex { re: 0.806847554, im: 0.590759702 }, Complex { re: 0.805031331, im: 0.593232295 },
  Complex { re: 0.803207531, im: 0.595699304 }, Complex { re: 0.801376172, im: 0.598160707 }, Complex { re: 0.799537269, im: 0.600616479 }, Complex { re: 0.797690841, im: 0.603066599 },
  Complex { re: 0.795836905, im: 0.605511041 }, Complex { re: 0.793975478, im: 0.607949785 }, Complex { re: 0.792106577, im: 0.610382806 }, Complex { re: 0.790230221, im: 0.612810082 },
  Complex { re: 0.788346428, im: 0.615231591 }, Complex { re: 0.786455214, im: 0.617647308 }, Complex { re: 0.784556597, im: 0.620057212 }, Complex { re: 0.782650596, im: 0.622461279 },
  Complex { re: 0.780737229, im: 0.624859488 }, Complex { re: 0.778816512, im: 0.627251815 }, Complex { re: 0.776888466, im: 0.629638239 }, Complex { re: 0.774953107, im: 0.632018736 },
  Complex { re: 0.773010453, im: 0.634393284 }, Complex { re: 0.771060524, im: 0.636761861 }, Complex { re: 0.769103338, im: 0.639124445 }, Complex { re: 0.767138912, im: 0.641481013 },
  Complex { re: 0.765167266, im: 0.643831543 }, Complex { re: 0.763188417, im: 0.646176013 }, Complex { re: 0.761202385, im: 0.648514401 }, Complex { re: 0.759209189, im: 0.650846685 },
  Complex { re: 0.757208847, im: 0.653172843 }, Complex { re: 0.755201377, im: 0.655492853 }, Complex { re: 0.753186799, im: 0.657806693 }, Complex { re: 0.751165132, im: 0.660114342 },
  Complex { re: 0.749136395, im: 0.662415778 }, Complex { re: 0.747100606, im: 0.664710978 }, Complex { re: 0.745057785, im: 0.666999922 }, Complex { re: 0.743007952, im: 0.669282588 },
  Complex { re: 0.740951125, im: 0.671558955 }, Complex { re: 0.738887324, im: 0.673829000 }, Complex { re: 0.736816569, im: 0.676092704 }, Complex { re: 0.734738878, im: 0.678350043 },
  Complex { re: 0.732654272, im: 0.680600998 }, Complex { re: 0.730562769, im: 0.682845546 }, Complex { re: 0.728464390, im: 0.685083668 }, Complex { re: 0.726359155, im: 0.687315341 },
  Complex { re: 0.724247083, im: 0.689540545 }, Complex { re: 0.722128194, im: 0.691759258 }, Complex { re: 0.720002508, im: 0.693971461 }, Complex { re: 0.717870045, im: 0.696177131 },
  Complex { re: 0.715730825, im: 0.698376249 }, Complex { re: 0.713584869, im: 0.700568794 }, Complex { re: 0.711432196, im: 0.702754744 }, Complex { re: 0.709272826, im: 0.704934080 },
  Complex { re: 0.707106781, im: 0.707106781 },
];
#[rustfmt::skip]
pub static SINE_TABLE_1024: [Complex<f32>; 513] = [

  Complex { re: 1.000000000, im: 0.000000000 }, Complex { re: 0.999998823, im: 0.001533980 }, Complex { re: 0.999995294, im: 0.003067957 }, Complex { re: 0.999989411, im: 0.004601926 },
  Complex { re: 0.999981175, im: 0.006135885 }, Complex { re: 0.999970586, im: 0.007669829 }, Complex { re: 0.999957645, im: 0.009203755 }, Complex { re: 0.999942350, im: 0.010737659 },
  Complex { re: 0.999924702, im: 0.012271538 }, Complex { re: 0.999904701, im: 0.013805389 }, Complex { re: 0.999882347, im: 0.015339206 }, Complex { re: 0.999857641, im: 0.016872988 },
  Complex { re: 0.999830582, im: 0.018406730 }, Complex { re: 0.999801170, im: 0.019940429 }, Complex { re: 0.999769405, im: 0.021474080 }, Complex { re: 0.999735288, im: 0.023007681 },
  Complex { re: 0.999698819, im: 0.024541229 }, Complex { re: 0.999659997, im: 0.026074718 }, Complex { re: 0.999618822, im: 0.027608146 }, Complex { re: 0.999575296, im: 0.029141509 },
  Complex { re: 0.999529418, im: 0.030674803 }, Complex { re: 0.999481187, im: 0.032208025 }, Complex { re: 0.999430605, im: 0.033741172 }, Complex { re: 0.999377670, im: 0.035274239 },
  Complex { re: 0.999322385, im: 0.036807223 }, Complex { re: 0.999264747, im: 0.038340120 }, Complex { re: 0.999204759, im: 0.039872928 }, Complex { re: 0.999142419, im: 0.041405641 },
  Complex { re: 0.999077728, im: 0.042938257 }, Complex { re: 0.999010686, im: 0.044470772 }, Complex { re: 0.998941293, im: 0.046003182 }, Complex { re: 0.998869550, im: 0.047535484 },
  Complex { re: 0.998795456, im: 0.049067674 }, Complex { re: 0.998719012, im: 0.050599749 }, Complex { re: 0.998640218, im: 0.052131705 }, Complex { re: 0.998559074, im: 0.053663538 },
  Complex { re: 0.998475581, im: 0.055195244 }, Complex { re: 0.998389737, im: 0.056726821 }, Complex { re: 0.998301545, im: 0.058258265 }, Complex { re: 0.998211003, im: 0.059789571 },
  Complex { re: 0.998118113, im: 0.061320736 }, Complex { re: 0.998022874, im: 0.062851758 }, Complex { re: 0.997925286, im: 0.064382631 }, Complex { re: 0.997825350, im: 0.065913353 },
  Complex { re: 0.997723067, im: 0.067443920 }, Complex { re: 0.997618435, im: 0.068974328 }, Complex { re: 0.997511456, im: 0.070504573 }, Complex { re: 0.997402130, im: 0.072034653 },
  Complex { re: 0.997290457, im: 0.073564564 }, Complex { re: 0.997176437, im: 0.075094301 }, Complex { re: 0.997060070, im: 0.076623861 }, Complex { re: 0.996941358, im: 0.078153242 },
  Complex { re: 0.996820299, im: 0.079682438 }, Complex { re: 0.996696895, im: 0.081211447 }, Complex { re: 0.996571146, im: 0.082740265 }, Complex { re: 0.996443051, im: 0.084268888 },
  Complex { re: 0.996312612, im: 0.085797312 }, Complex { re: 0.996179829, im: 0.087325535 }, Complex { re: 0.996044701, im: 0.088853553 }, Complex { re: 0.995907229, im: 0.090381361 },
  Complex { re: 0.995767414, im: 0.091908956 }, Complex { re: 0.995625256, im: 0.093436336 }, Complex { re: 0.995480755, im: 0.094963495 }, Complex { re: 0.995333912, im: 0.096490431 },
  Complex { re: 0.995184727, im: 0.098017140 }, Complex { re: 0.995033199, im: 0.099543619 }, Complex { re: 0.994879331, im: 0.101069863 }, Complex { re: 0.994723121, im: 0.102595869 },
  Complex { re: 0.994564571, im: 0.104121634 }, Complex { re: 0.994403680, im: 0.105647154 }, Complex { re: 0.994240449, im: 0.107172425 }, Complex { re: 0.994074879, im: 0.108697444 },
  Complex { re: 0.993906970, im: 0.110222207 }, Complex { re: 0.993736722, im: 0.111746711 }, Complex { re: 0.993564136, im: 0.113270952 }, Complex { re: 0.993389211, im: 0.114794927 },
  Complex { re: 0.993211949, im: 0.116318631 }, Complex { re: 0.993032350, im: 0.117842062 }, Complex { re: 0.992850414, im: 0.119365215 }, Complex { re: 0.992666142, im: 0.120888087 },
  Complex { re: 0.992479535, im: 0.122410675 }, Complex { re: 0.992290591, im: 0.123932975 }, Complex { re: 0.992099313, im: 0.125454983 }, Complex { re: 0.991905700, im: 0.126976696 },
  Complex { re: 0.991709754, im: 0.128498111 }, Complex { re: 0.991511473, im: 0.130019223 }, Complex { re: 0.991310860, im: 0.131540029 }, Complex { re: 0.991107914, im: 0.133060525 },
  Complex { re: 0.990902635, im: 0.134580709 }, Complex { re: 0.990695025, im: 0.136100575 }, Complex { re: 0.990485084, im: 0.137620122 }, Complex { re: 0.990272812, im: 0.139139344 },
  Complex { re: 0.990058210, im: 0.140658239 }, Complex { re: 0.989841278, im: 0.142176804 }, Complex { re: 0.989622017, im: 0.143695033 }, Complex { re: 0.989400428, im: 0.145212925 },
  Complex { re: 0.989176510, im: 0.146730474 }, Complex { re: 0.988950265, im: 0.148247679 }, Complex { re: 0.988721692, im: 0.149764535 }, Complex { re: 0.988490793, im: 0.151281038 },
  Complex { re: 0.988257568, im: 0.152797185 }, Complex { re: 0.988022017, im: 0.154312973 }, Complex { re: 0.987784142, im: 0.155828398 }, Complex { re: 0.987543942, im: 0.157343456 },
  Complex { re: 0.987301418, im: 0.158858143 }, Complex { re: 0.987056571, im: 0.160372457 }, Complex { re: 0.986809402, im: 0.161886394 }, Complex { re: 0.986559910, im: 0.163399949 },
  Complex { re: 0.986308097, im: 0.164913120 }, Complex { re: 0.986053963, im: 0.166425904 }, Complex { re: 0.985797509, im: 0.167938295 }, Complex { re: 0.985538735, im: 0.169450291 },
  Complex { re: 0.985277642, im: 0.170961889 }, Complex { re: 0.985014231, im: 0.172473084 }, Complex { re: 0.984748502, im: 0.173983873 }, Complex { re: 0.984480455, im: 0.175494253 },
  Complex { re: 0.984210092, im: 0.177004220 }, Complex { re: 0.983937413, im: 0.178513771 }, Complex { re: 0.983662419, im: 0.180022901 }, Complex { re: 0.983385110, im: 0.181531608 },
  Complex { re: 0.983105487, im: 0.183039888 }, Complex { re: 0.982823551, im: 0.184547737 }, Complex { re: 0.982539302, im: 0.186055152 }, Complex { re: 0.982252741, im: 0.187562129 },
  Complex { re: 0.981963869, im: 0.189068664 }, Complex { re: 0.981672686, im: 0.190574755 }, Complex { re: 0.981379193, im: 0.192080397 }, Complex { re: 0.981083391, im: 0.193585587 },
  Complex { re: 0.980785280, im: 0.195090322 }, Complex { re: 0.980484862, im: 0.196594598 }, Complex { re: 0.980182136, im: 0.198098411 }, Complex { re: 0.979877104, im: 0.199601758 },
  Complex { re: 0.979569766, im: 0.201104635 }, Complex { re: 0.979260123, im: 0.202607039 }, Complex { re: 0.978948175, im: 0.204108966 }, Complex { re: 0.978633924, im: 0.205610413 },
  Complex { re: 0.978317371, im: 0.207111376 }, Complex { re: 0.977998515, im: 0.208611852 }, Complex { re: 0.977677358, im: 0.210111837 }, Complex { re: 0.977353900, im: 0.211611327 },
  Complex { re: 0.977028143, im: 0.213110320 }, Complex { re: 0.976700086, im: 0.214608811 }, Complex { re: 0.976369731, im: 0.216106797 }, Complex { re: 0.976037079, im: 0.217604275 },
  Complex { re: 0.975702130, im: 0.219101240 }, Complex { re: 0.975364885, im: 0.220597690 }, Complex { re: 0.975025345, im: 0.222093621 }, Complex { re: 0.974683511, im: 0.223589029 },
  Complex { re: 0.974339383, im: 0.225083911 }, Complex { re: 0.973992962, im: 0.226578264 }, Complex { re: 0.973644250, im: 0.228072083 }, Complex { re: 0.973293246, im: 0.229565366 },
  Complex { re: 0.972939952, im: 0.231058108 }, Complex { re: 0.972584369, im: 0.232550307 }, Complex { re: 0.972226497, im: 0.234041959 }, Complex { re: 0.971866337, im: 0.235533059 },
  Complex { re: 0.971503891, im: 0.237023606 }, Complex { re: 0.971139158, im: 0.238513595 }, Complex { re: 0.970772141, im: 0.240003022 }, Complex { re: 0.970402839, im: 0.241491885 },
  Complex { re: 0.970031253, im: 0.242980180 }, Complex { re: 0.969657385, im: 0.244467903 }, Complex { re: 0.969281235, im: 0.245955050 }, Complex { re: 0.968902805, im: 0.247441619 },
  Complex { re: 0.968522094, im: 0.248927606 }, Complex { re: 0.968139105, im: 0.250413007 }, Complex { re: 0.967753837, im: 0.251897818 }, Complex { re: 0.967366292, im: 0.253382037 },
  Complex { re: 0.966976471, im: 0.254865660 }, Complex { re: 0.966584374, im: 0.256348682 }, Complex { re: 0.966190003, im: 0.257831102 }, Complex { re: 0.965793359, im: 0.259312915 },
  Complex { re: 0.965394442, im: 0.260794118 }, Complex { re: 0.964993253, im: 0.262274707 }, Complex { re: 0.964589793, im: 0.263754679 }, Complex { re: 0.964184064, im: 0.265234030 },
  Complex { re: 0.963776066, im: 0.266712757 }, Complex { re: 0.963365800, im: 0.268190857 }, Complex { re: 0.962953267, im: 0.269668326 }, Complex { re: 0.962538468, im: 0.271145160 },
  Complex { re: 0.962121404, im: 0.272621355 }, Complex { re: 0.961702077, im: 0.274096910 }, Complex { re: 0.961280486, im: 0.275571819 }, Complex { re: 0.960856633, im: 0.277046080 },
  Complex { re: 0.960430519, im: 0.278519689 }, Complex { re: 0.960002146, im: 0.279992643 }, Complex { re: 0.959571513, im: 0.281464938 }, Complex { re: 0.959138622, im: 0.282936570 },
  Complex { re: 0.958703475, im: 0.284407537 }, Complex { re: 0.958266071, im: 0.285877835 }, Complex { re: 0.957826413, im: 0.287347460 }, Complex { re: 0.957384501, im: 0.288816408 },
  Complex { re: 0.956940336, im: 0.290284677 }, Complex { re: 0.956493919, im: 0.291752263 }, Complex { re: 0.956045251, im: 0.293219163 }, Complex { re: 0.955594334, im: 0.294685372 },
  Complex { re: 0.955141168, im: 0.296150888 }, Complex { re: 0.954685755, im: 0.297615707 }, Complex { re: 0.954228095, im: 0.299079826 }, Complex { re: 0.953768190, im: 0.300543241 },
  Complex { re: 0.953306040, im: 0.302005949 }, Complex { re: 0.952841648, im: 0.303467947 }, Complex { re: 0.952375013, im: 0.304929230 }, Complex { re: 0.951906137, im: 0.306389795 },
  Complex { re: 0.951435021, im: 0.307849640 }, Complex { re: 0.950961666, im: 0.309308760 }, Complex { re: 0.950486074, im: 0.310767153 }, Complex { re: 0.950008245, im: 0.312224814 },
  Complex { re: 0.949528181, im: 0.313681740 }, Complex { re: 0.949045882, im: 0.315137929 }, Complex { re: 0.948561350, im: 0.316593376 }, Complex { re: 0.948074586, im: 0.318048077 },
  Complex { re: 0.947585591, im: 0.319502031 }, Complex { re: 0.947094366, im: 0.320955232 }, Complex { re: 0.946600913, im: 0.322407679 }, Complex { re: 0.946105232, im: 0.323859367 },
  Complex { re: 0.945607325, im: 0.325310292 }, Complex { re: 0.945107193, im: 0.326760452 }, Complex { re: 0.944604837, im: 0.328209844 }, Complex { re: 0.944100258, im: 0.329658463 },
  Complex { re: 0.943593458, im: 0.331106306 }, Complex { re: 0.943084437, im: 0.332553370 }, Complex { re: 0.942573198, im: 0.333999651 }, Complex { re: 0.942059740, im: 0.335445147 },
  Complex { re: 0.941544065, im: 0.336889853 }, Complex { re: 0.941026175, im: 0.338333767 }, Complex { re: 0.940506071, im: 0.339776884 }, Complex { re: 0.939983753, im: 0.341219202 },
  Complex { re: 0.939459224, im: 0.342660717 }, Complex { re: 0.938932484, im: 0.344101426 }, Complex { re: 0.938403534, im: 0.345541325 }, Complex { re: 0.937872376, im: 0.346980411 },
  Complex { re: 0.937339012, im: 0.348418680 }, Complex { re: 0.936803442, im: 0.349856130 }, Complex { re: 0.936265667, im: 0.351292756 }, Complex { re: 0.935725689, im: 0.352728556 },
  Complex { re: 0.935183510, im: 0.354163525 }, Complex { re: 0.934639130, im: 0.355597662 }, Complex { re: 0.934092550, im: 0.357030961 }, Complex { re: 0.933543773, im: 0.358463421 },
  Complex { re: 0.932992799, im: 0.359895037 }, Complex { re: 0.932439629, im: 0.361325806 }, Complex { re: 0.931884266, im: 0.362755724 }, Complex { re: 0.931326709, im: 0.364184790 },
  Complex { re: 0.930766961, im: 0.365612998 }, Complex { re: 0.930205023, im: 0.367040346 }, Complex { re: 0.929640896, im: 0.368466830 }, Complex { re: 0.929074581, im: 0.369892447 },
  Complex { re: 0.928506080, im: 0.371317194 }, Complex { re: 0.927935395, im: 0.372741067 }, Complex { re: 0.927362526, im: 0.374164063 }, Complex { re: 0.926787474, im: 0.375586178 },
  Complex { re: 0.926210242, im: 0.377007410 }, Complex { re: 0.925630831, im: 0.378427755 }, Complex { re: 0.925049241, im: 0.379847209 }, Complex { re: 0.924465474, im: 0.381265769 },
  Complex { re: 0.923879533, im: 0.382683432 }, Complex { re: 0.923291417, im: 0.384100195 }, Complex { re: 0.922701128, im: 0.385516054 }, Complex { re: 0.922108669, im: 0.386931006 },
  Complex { re: 0.921514039, im: 0.388345047 }, Complex { re: 0.920917242, im: 0.389758174 }, Complex { re: 0.920318277, im: 0.391170384 }, Complex { re: 0.919717146, im: 0.392581674 },
  Complex { re: 0.919113852, im: 0.393992040 }, Complex { re: 0.918508394, im: 0.395401479 }, Complex { re: 0.917900776, im: 0.396809987 }, Complex { re: 0.917290997, im: 0.398217562 },
  Complex { re: 0.916679060, im: 0.399624200 }, Complex { re: 0.916064966, im: 0.401029897 }, Complex { re: 0.915448716, im: 0.402434651 }, Complex { re: 0.914830312, im: 0.403838458 },
  Complex { re: 0.914209756, im: 0.405241314 }, Complex { re: 0.913587048, im: 0.406643217 }, Complex { re: 0.912962190, im: 0.408044163 }, Complex { re: 0.912335185, im: 0.409444149 },
  Complex { re: 0.911706032, im: 0.410843171 }, Complex { re: 0.911074734, im: 0.412241227 }, Complex { re: 0.910441292, im: 0.413638312 }, Complex { re: 0.909805708, im: 0.415034424 },
  Complex { re: 0.909167983, im: 0.416429560 }, Complex { re: 0.908528119, im: 0.417823716 }, Complex { re: 0.907886116, im: 0.419216888 }, Complex { re: 0.907241978, im: 0.420609074 },
  Complex { re: 0.906595705, im: 0.422000271 }, Complex { re: 0.905947298, im: 0.423390474 }, Complex { re: 0.905296759, im: 0.424779681 }, Complex { re: 0.904644091, im: 0.426167889 },
  Complex { re: 0.903989293, im: 0.427555093 }, Complex { re: 0.903332368, im: 0.428941292 }, Complex { re: 0.902673318, im: 0.430326481 }, Complex { re: 0.902012144, im: 0.431710658 },
  Complex { re: 0.901348847, im: 0.433093819 }, Complex { re: 0.900683429, im: 0.434475961 }, Complex { re: 0.900015892, im: 0.435857080 }, Complex { re: 0.899346237, im: 0.437237174 },
  Complex { re: 0.898674466, im: 0.438616239 }, Complex { re: 0.898000580, im: 0.439994271 }, Complex { re: 0.897324581, im: 0.441371269 }, Complex { re: 0.896646470, im: 0.442747228 },
  Complex { re: 0.895966250, im: 0.444122145 }, Complex { re: 0.895283921, im: 0.445496017 }, Complex { re: 0.894599486, im: 0.446868840 }, Complex { re: 0.893912945, im: 0.448240612 },
  Complex { re: 0.893224301, im: 0.449611330 }, Complex { re: 0.892533555, im: 0.450980989 }, Complex { re: 0.891840709, im: 0.452349587 }, Complex { re: 0.891145765, im: 0.453717121 },
  Complex { re: 0.890448723, im: 0.455083587 }, Complex { re: 0.889749586, im: 0.456448982 }, Complex { re: 0.889048356, im: 0.457813304 }, Complex { re: 0.888345033, im: 0.459176548 },
  Complex { re: 0.887639620, im: 0.460538711 }, Complex { re: 0.886932119, im: 0.461899791 }, Complex { re: 0.886222530, im: 0.463259784 }, Complex { re: 0.885510856, im: 0.464618686 },
  Complex { re: 0.884797098, im: 0.465976496 }, Complex { re: 0.884081259, im: 0.467333209 }, Complex { re: 0.883363339, im: 0.468688822 }, Complex { re: 0.882643340, im: 0.470043332 },
  Complex { re: 0.881921264, im: 0.471396737 }, Complex { re: 0.881197113, im: 0.472749032 }, Complex { re: 0.880470889, im: 0.474100215 }, Complex { re: 0.879742593, im: 0.475450282 },
  Complex { re: 0.879012226, im: 0.476799230 }, Complex { re: 0.878279792, im: 0.478147056 }, Complex { re: 0.877545290, im: 0.479493758 }, Complex { re: 0.876808724, im: 0.480839331 },
  Complex { re: 0.876070094, im: 0.482183772 }, Complex { re: 0.875329403, im: 0.483527079 }, Complex { re: 0.874586652, im: 0.484869248 }, Complex { re: 0.873841843, im: 0.486210276 },
  Complex { re: 0.873094978, im: 0.487550160 }, Complex { re: 0.872346059, im: 0.488888897 }, Complex { re: 0.871595087, im: 0.490226483 }, Complex { re: 0.870842063, im: 0.491562916 },
  Complex { re: 0.870086991, im: 0.492898192 }, Complex { re: 0.869329871, im: 0.494232309 }, Complex { re: 0.868570706, im: 0.495565262 }, Complex { re: 0.867809497, im: 0.496897049 },
  Complex { re: 0.867046246, im: 0.498227667 }, Complex { re: 0.866280954, im: 0.499557113 }, Complex { re: 0.865513624, im: 0.500885383 }, Complex { re: 0.864744258, im: 0.502212474 },
  Complex { re: 0.863972856, im: 0.503538384 }, Complex { re: 0.863199422, im: 0.504863109 }, Complex { re: 0.862423956, im: 0.506186645 }, Complex { re: 0.861646461, im: 0.507508991 },
  Complex { re: 0.860866939, im: 0.508830143 }, Complex { re: 0.860085390, im: 0.510150097 }, Complex { re: 0.859301818, im: 0.511468850 }, Complex { re: 0.858516224, im: 0.512786401 },
  Complex { re: 0.857728610, im: 0.514102744 }, Complex { re: 0.856938977, im: 0.515417878 }, Complex { re: 0.856147328, im: 0.516731799 }, Complex { re: 0.855353665, im: 0.518044504 },
  Complex { re: 0.854557988, im: 0.519355990 }, Complex { re: 0.853760301, im: 0.520666254 }, Complex { re: 0.852960605, im: 0.521975293 }, Complex { re: 0.852158902, im: 0.523283103 },
  Complex { re: 0.851355193, im: 0.524589683 }, Complex { re: 0.850549481, im: 0.525895027 }, Complex { re: 0.849741768, im: 0.527199135 }, Complex { re: 0.848932055, im: 0.528502002 },
  Complex { re: 0.848120345, im: 0.529803625 }, Complex { re: 0.847306639, im: 0.531104001 }, Complex { re: 0.846490939, im: 0.532403128 }, Complex { re: 0.845673247, im: 0.533701002 },
  Complex { re: 0.844853565, im: 0.534997620 }, Complex { re: 0.844031895, im: 0.536292979 }, Complex { re: 0.843208240, im: 0.537587076 }, Complex { re: 0.842382600, im: 0.538879909 },
  Complex { re: 0.841554977, im: 0.540171473 }, Complex { re: 0.840725375, im: 0.541461766 }, Complex { re: 0.839893794, im: 0.542750785 }, Complex { re: 0.839060237, im: 0.544038527 },
  Complex { re: 0.838224706, im: 0.545324988 }, Complex { re: 0.837387202, im: 0.546610167 }, Complex { re: 0.836547727, im: 0.547894059 }, Complex { re: 0.835706284, im: 0.549176662 },
  Complex { re: 0.834862875, im: 0.550457973 }, Complex { re: 0.834017501, im: 0.551737988 }, Complex { re: 0.833170165, im: 0.553016706 }, Complex { re: 0.832320868, im: 0.554294121 },
  Complex { re: 0.831469612, im: 0.555570233 }, Complex { re: 0.830616400, im: 0.556845037 }, Complex { re: 0.829761234, im: 0.558118531 }, Complex { re: 0.828904115, im: 0.559390712 },
  Complex { re: 0.828045045, im: 0.560661576 }, Complex { re: 0.827184027, im: 0.561931121 }, Complex { re: 0.826321063, im: 0.563199344 }, Complex { re: 0.825456154, im: 0.564466242 },
  Complex { re: 0.824589303, im: 0.565731811 }, Complex { re: 0.823720511, im: 0.566996049 }, Complex { re: 0.822849781, im: 0.568258953 }, Complex { re: 0.821977115, im: 0.569520519 },
  Complex { re: 0.821102515, im: 0.570780746 }, Complex { re: 0.820225983, im: 0.572039629 }, Complex { re: 0.819347520, im: 0.573297167 }, Complex { re: 0.818467130, im: 0.574553355 },
  Complex { re: 0.817584813, im: 0.575808191 }, Complex { re: 0.816700573, im: 0.577061673 }, Complex { re: 0.815814411, im: 0.578313796 }, Complex { re: 0.814926329, im: 0.579564559 },
  Complex { re: 0.814036330, im: 0.580813958 }, Complex { re: 0.813144415, im: 0.582061990 }, Complex { re: 0.812250587, im: 0.583308653 }, Complex { re: 0.811354847, im: 0.584553943 },
  Complex { re: 0.810457198, im: 0.585797857 }, Complex { re: 0.809557642, im: 0.587040394 }, Complex { re: 0.808656182, im: 0.588281548 }, Complex { re: 0.807752818, im: 0.589521319 },
  Complex { re: 0.806847554, im: 0.590759702 }, Complex { re: 0.805940391, im: 0.591996695 }, Complex { re: 0.805031331, im: 0.593232295 }, Complex { re: 0.804120377, im: 0.594466499 },
  Complex { re: 0.803207531, im: 0.595699304 }, Complex { re: 0.802292796, im: 0.596930708 }, Complex { re: 0.801376172, im: 0.598160707 }, Complex { re: 0.800457662, im: 0.599389298 },
  Complex { re: 0.799537269, im: 0.600616479 }, Complex { re: 0.798614995, im: 0.601842247 }, Complex { re: 0.797690841, im: 0.603066599 }, Complex { re: 0.796764810, im: 0.604289531 },
  Complex { re: 0.795836905, im: 0.605511041 }, Complex { re: 0.794907126, im: 0.606731127 }, Complex { re: 0.793975478, im: 0.607949785 }, Complex { re: 0.793041960, im: 0.609167012 },
  Complex { re: 0.792106577, im: 0.610382806 }, Complex { re: 0.791169330, im: 0.611597164 }, Complex { re: 0.790230221, im: 0.612810082 }, Complex { re: 0.789289253, im: 0.614021559 },
  Complex { re: 0.788346428, im: 0.615231591 }, Complex { re: 0.787401747, im: 0.616440175 }, Complex { re: 0.786455214, im: 0.617647308 }, Complex { re: 0.785506830, im: 0.618852988 },
  Complex { re: 0.784556597, im: 0.620057212 }, Complex { re: 0.783604519, im: 0.621259977 }, Complex { re: 0.782650596, im: 0.622461279 }, Complex { re: 0.781694832, im: 0.623661118 },
  Complex { re: 0.780737229, im: 0.624859488 }, Complex { re: 0.779777788, im: 0.626056388 }, Complex { re: 0.778816512, im: 0.627251815 }, Complex { re: 0.777853404, im: 0.628445767 },
  Complex { re: 0.776888466, im: 0.629638239 }, Complex { re: 0.775921699, im: 0.630829230 }, Complex { re: 0.774953107, im: 0.632018736 }, Complex { re: 0.773982691, im: 0.633206755 },
  Complex { re: 0.773010453, im: 0.634393284 }, Complex { re: 0.772036397, im: 0.635578320 }, Complex { re: 0.771060524, im: 0.636761861 }, Complex { re: 0.770082837, im: 0.637943904 },
  Complex { re: 0.769103338, im: 0.639124445 }, Complex { re: 0.768122029, im: 0.640303482 }, Complex { re: 0.767138912, im: 0.641481013 }, Complex { re: 0.766153990, im: 0.642657034 },
  Complex { re: 0.765167266, im: 0.643831543 }, Complex { re: 0.764178741, im: 0.645004537 }, Complex { re: 0.763188417, im: 0.646176013 }, Complex { re: 0.762196298, im: 0.647345969 },
  Complex { re: 0.761202385, im: 0.648514401 }, Complex { re: 0.760206682, im: 0.649681307 }, Complex { re: 0.759209189, im: 0.650846685 }, Complex { re: 0.758209910, im: 0.652010531 },
  Complex { re: 0.757208847, im: 0.653172843 }, Complex { re: 0.756206001, im: 0.654333618 }, Complex { re: 0.755201377, im: 0.655492853 }, Complex { re: 0.754194975, im: 0.656650546 },
  Complex { re: 0.753186799, im: 0.657806693 }, Complex { re: 0.752176850, im: 0.658961293 }, Complex { re: 0.751165132, im: 0.660114342 }, Complex { re: 0.750151646, im: 0.661265838 },
  Complex { re: 0.749136395, im: 0.662415778 }, Complex { re: 0.748119380, im: 0.663564159 }, Complex { re: 0.747100606, im: 0.664710978 }, Complex { re: 0.746080074, im: 0.665856234 },
  Complex { re: 0.745057785, im: 0.666999922 }, Complex { re: 0.744033744, im: 0.668142041 }, Complex { re: 0.743007952, im: 0.669282588 }, Complex { re: 0.741980412, im: 0.670421560 },
  Complex { re: 0.740951125, im: 0.671558955 }, Complex { re: 0.739920095, im: 0.672694769 }, Complex { re: 0.738887324, im: 0.673829000 }, Complex { re: 0.737852815, im: 0.674961646 },
  Complex { re: 0.736816569, im: 0.676092704 }, Complex { re: 0.735778589, im: 0.677222170 }, Complex { re: 0.734738878, im: 0.678350043 }, Complex { re: 0.733697438, im: 0.679476320 },
  Complex { re: 0.732654272, im: 0.680600998 }, Complex { re: 0.731609381, im: 0.681724074 }, Complex { re: 0.730562769, im: 0.682845546 }, Complex { re: 0.729514438, im: 0.683965412 },
  Complex { re: 0.728464390, im: 0.685083668 }, Complex { re: 0.727412629, im: 0.686200312 }, Complex { re: 0.726359155, im: 0.687315341 }, Complex { re: 0.725303972, im: 0.688428753 },
  Complex { re: 0.724247083, im: 0.689540545 }, Complex { re: 0.723188489, im: 0.690650714 }, Complex { re: 0.722128194, im: 0.691759258 }, Complex { re: 0.721066199, im: 0.692866175 },
  Complex { re: 0.720002508, im: 0.693971461 }, Complex { re: 0.718937122, im: 0.695075114 }, Complex { re: 0.717870045, im: 0.696177131 }, Complex { re: 0.716801279, im: 0.697277511 },
  Complex { re: 0.715730825, im: 0.698376249 }, Complex { re: 0.714658688, im: 0.699473345 }, Complex { re: 0.713584869, im: 0.700568794 }, Complex { re: 0.712509371, im: 0.701662595 },
  Complex { re: 0.711432196, im: 0.702754744 }, Complex { re: 0.710353347, im: 0.703845241 }, Complex { re: 0.709272826, im: 0.704934080 }, Complex { re: 0.708190637, im: 0.706021261 },
  Complex { re: 0.707106781, im: 0.707106781 },
];
