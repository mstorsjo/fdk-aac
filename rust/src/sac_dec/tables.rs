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
//! Tables used for SAC decoder.

use super::{
    common::FreqResolution,
    constants::{N_CLD, N_ICC, N_IPD},
};

/// Arbitrary matrix elements.
#[rustfmt::skip]
pub(super) static H11_NC: [[f32; N_ICC]; N_CLD] =
[
    [0.0000000316, 0.0000000296, 0.0000000266, 0.0000000190, 0.0000000116,
     0.0000000000, -0.0000000186, -0.0000000313],
    [0.0056233243, 0.0052728835, 0.0047394098, 0.0033992692, 0.0020946222,
     0.0000316215, -0.0032913829, -0.0055664564],
    [0.0099994997, 0.0093815643, 0.0084402543, 0.0060722125, 0.0037622179,
     0.0000999898, -0.0058238208, -0.0098974844],
    [0.0177799836, 0.0166974831, 0.0150465844, 0.0108831404, 0.0068073822,
     0.0003161267, -0.0102626514, -0.0175957214],
    [0.0316069759, 0.0297324844, 0.0268681273, 0.0196138974, 0.0124691967,
     0.0009989988, -0.0179452803, -0.0312700421],
    [0.0561454296, 0.0529650487, 0.0480896905, 0.0356564634, 0.0232860073,
     0.0031523081, -0.0309029408, -0.0555154830],
    [0.0791834071, 0.0748842582, 0.0682762116, 0.0513241664, 0.0343080349,
     0.0062700072, -0.0422340371, -0.0782499388],
    [0.1115021780, 0.1057924852, 0.0969873071, 0.0742305145, 0.0511277616,
     0.0124327289, -0.0566596612, -0.1100896299],
    [0.1565355062, 0.1491366178, 0.1376826316, 0.1078186408, 0.0770794004,
     0.0245033558, -0.0735980421, -0.1543303132],
    [0.2184644639, 0.2091979682, 0.1947948188, 0.1568822265, 0.1172478944,
     0.0477267131, -0.0899507254, -0.2148526460],
    [0.3015113473, 0.2904391289, 0.2731673419, 0.2273024023, 0.1786239147,
     0.0909090787, -0.0964255333, -0.2951124907],
    [0.3698741496, 0.3578284085, 0.3390066922, 0.2888108492, 0.2351117432,
     0.1368068755, -0.0850296095, -0.3597966135],
    [0.4480624795, 0.4354025424, 0.4156077504, 0.3627120256, 0.3058823943,
     0.2007599771, -0.0484020934, -0.4304940701],
    [0.5336171389, 0.5208471417, 0.5008935928, 0.4476420581, 0.3905044496,
     0.2847472429, 0.0276676007, -0.4966579080],
    [0.6219832301, 0.6096963882, 0.5905415416, 0.5396950245, 0.4856070578,
     0.3868631124, 0.1531652957, -0.5045361519],
    [0.7071067691, 0.6958807111, 0.6784504056, 0.6326373219, 0.5847306848,
     0.4999999702, 0.3205464482, 0.0500000045],
    [0.7830305696, 0.7733067870, 0.7582961321, 0.7194055915, 0.6797705293,
     0.6131368876, 0.4997332692, 0.6934193969],
    [0.8457261920, 0.8377274871, 0.8254694939, 0.7942851782, 0.7635439038,
     0.7152527571, 0.6567122936, 0.8229061961],
    [0.8940021992, 0.8877248168, 0.8781855106, 0.8544237614, 0.8318918347,
     0.7992399335, 0.7751275301, 0.8853276968],
    [0.9290818572, 0.9243524075, 0.9172304869, 0.8998877406, 0.8841174841,
     0.8631930947, 0.8565139771, 0.9251161218],
    [0.9534626007, 0.9500193000, 0.9448821545, 0.9326565266, 0.9220023751,
     0.9090909362, 0.9096591473, 0.9514584541],
    [0.9758449197, 0.9738122821, 0.9708200693, 0.9639287591, 0.9582763910,
     0.9522733092, 0.9553207159, 0.9750427008],
    [0.9876723289, 0.9865267277, 0.9848603010, 0.9811310172, 0.9782302976,
     0.9754966497, 0.9779621363, 0.9873252511],
    [0.9937641621, 0.9931397438, 0.9922404289, 0.9902750254, 0.9888116717,
     0.9875672460, 0.9891131520, 0.9936066866],
    [0.9968600869, 0.9965277910, 0.9960530400, 0.9950347543, 0.9943022728,
     0.9937300086, 0.9946073294, 0.9967863560],
    [0.9984226227, 0.9982488155, 0.9980020523, 0.9974802136, 0.9971146584,
     0.9968476892, 0.9973216057, 0.9983873963],
    [0.9995003939, 0.9994428754, 0.9993617535, 0.9991930723, 0.9990783334,
     0.9990010262, 0.9991616607, 0.9994897842],
    [0.9998419285, 0.9998232722, 0.9997970462, 0.9997430444, 0.9997069836,
     0.9996838570, 0.9997364879, 0.9998386502],
    [0.9999499917, 0.9999440312, 0.9999356270, 0.9999184012, 0.9999070764,
     0.9998999834, 0.9999169707, 0.9999489784],
    [0.9999842048, 0.9999822974, 0.9999796152, 0.9999741912, 0.9999706149,
     0.9999684095, 0.9999738336, 0.9999839067],
    [1.0000000000, 1.0000000000, 1.0000000000, 1.0000000000, 1.0000000000,
     1.0000000000, 1.0000000000, 1.0000000000]
];

/// Arbitrary matrix elements.
#[rustfmt::skip]
pub(super) static H12_NC: [[f32; N_ICC]; N_CLD] =
[
    [0.0000000000, 0.0000000110, 0.0000000171, 0.0000000253, 0.0000000294,
     0.0000000316, 0.0000000256, 0.0000000045],
    [0.0000000000, 0.0019540924, 0.0030265113, 0.0044795922, 0.0052186525,
     0.0056232354, 0.0045594489, 0.0007977085],
    [0.0000000000, 0.0034606720, 0.0053620986, 0.0079446984, 0.0092647560,
     0.0099989995, 0.0081285369, 0.0014247064],
    [0.0000000000, 0.0061091618, 0.0094724922, 0.0140600521, 0.0164252054,
     0.0177771728, 0.0145191532, 0.0025531140],
    [0.0000000000, 0.0107228858, 0.0166464616, 0.0247849934, 0.0290434174,
     0.0315911844, 0.0260186065, 0.0046027615],
    [0.0000000000, 0.0186282862, 0.0289774220, 0.0433696397, 0.0510888547,
     0.0560568646, 0.0468755551, 0.0083869267],
    [0.0000000000, 0.0257363543, 0.0401044972, 0.0602979437, 0.0713650510,
     0.0789347738, 0.0669798329, 0.0121226767],
    [0.0000000000, 0.0352233723, 0.0550108925, 0.0832019597, 0.0990892947,
     0.1108068749, 0.0960334241, 0.0176920593],
    [0.0000000000, 0.0475566536, 0.0744772255, 0.1134835035, 0.1362429112,
     0.1546057910, 0.1381545961, 0.0261824392],
    [0.0000000000, 0.0629518181, 0.0989024863, 0.1520351619, 0.1843357086,
     0.2131874412, 0.1990868896, 0.0395608991],
    [0.0000000000, 0.0809580907, 0.1276271492, 0.1980977356, 0.2429044843,
     0.2874797881, 0.2856767476, 0.0617875643],
    [0.0000000000, 0.0936254337, 0.1479234397, 0.2310739607, 0.2855334580,
     0.3436433673, 0.3599678576, 0.0857512727],
    [0.0000000000, 0.1057573780, 0.1674221754, 0.2630588412, 0.3274079263,
     0.4005688727, 0.4454404712, 0.1242370531],
    [0.0000000000, 0.1160409302, 0.1839915067, 0.2904545665, 0.3636667728,
     0.4512939751, 0.5328993797, 0.1951362640],
    [0.0000000000, 0.1230182052, 0.1952532977, 0.3091802597, 0.3886501491,
     0.4870318770, 0.6028295755, 0.3637395203],
    [0.0000000000, 0.1254990250, 0.1992611140, 0.3158638775, 0.3976053298,
     0.5000000000, 0.6302776933, 0.7053368092],
    [0.0000000000, 0.1230182052, 0.1952533126, 0.3091802597, 0.3886501491,
     0.4870319068, 0.6028295755, 0.3637394905],
    [0.0000000000, 0.1160409302, 0.1839915216, 0.2904545665, 0.3636668026,
     0.4512939751, 0.5328993797, 0.1951362044],
    [0.0000000000, 0.1057573855, 0.1674221754, 0.2630588710, 0.3274079263,
     0.4005688727, 0.4454405010, 0.1242370382],
    [0.0000000000, 0.0936254337, 0.1479234397, 0.2310739607, 0.2855334580,
     0.3436433673, 0.3599678576, 0.0857512653],
    [0.0000000000, 0.0809580907, 0.1276271492, 0.1980977207, 0.2429044843,
     0.2874797881, 0.2856767476, 0.0617875606],
    [0.0000000000, 0.0629518107, 0.0989024863, 0.1520351619, 0.1843357235,
     0.2131874412, 0.1990868896, 0.0395609401],
    [0.0000000000, 0.0475566462, 0.0744772255, 0.1134835184, 0.1362429112,
     0.1546057761, 0.1381545961, 0.0261824802],
    [0.0000000000, 0.0352233797, 0.0550108962, 0.0832019448, 0.0990892798,
     0.1108068526, 0.0960334465, 0.0176920686],
    [0.0000000000, 0.0257363524, 0.0401044935, 0.0602979474, 0.0713650808,
     0.0789347589, 0.0669797957, 0.0121226516],
    [0.0000000000, 0.0186282881, 0.0289774258, 0.0433696248, 0.0510888547,
     0.0560568906, 0.0468755886, 0.0083869714],
    [0.0000000000, 0.0107228830, 0.0166464727, 0.0247849822, 0.0290434249,
     0.0315911621, 0.0260186475, 0.0046027377],
    [0.0000000000, 0.0061091576, 0.0094724894, 0.0140600465, 0.0164251942,
     0.0177771524, 0.0145191504, 0.0025530567],
    [0.0000000000, 0.0034606743, 0.0053620976, 0.0079446994, 0.0092647672,
     0.0099990256, 0.0081285043, 0.0014247177],
    [0.0000000000, 0.0019540912, 0.0030265225, 0.0044795908, 0.0052186381,
     0.0056232223, 0.0045594289, 0.0007977359],
    [0.0000000000, 0.0000000149, 0.0000000298, 0.0000000298, 0.0000000000,
     0.0000000596, 0.0000000000, 0.0000000000]
];

pub(super) static DEQUANT_ICC: [f32; N_ICC] = [
    1.00000, 0.9370, 0.84118, 0.60092, 0.36764, 0.0000, -0.58900, -0.9900,
];

#[rustfmt::skip]
pub(super) static _DEQUANT_CLD: [f32; N_CLD] = [
    -150.0, -45.0, -40.0, -35.0, -30.0, -25.0, -22.0, -19.0,
    -16.0,  -13.0, -10.0, -8.0,  -6.0,  -4.0,  -2.0,  0.0,
    2.0,    4.0,   6.0,   8.0,   10.0,  13.0,  16.0,  19.0,
    22.0,   25.0,  30.0,  35.0,  40.0,  45.0,  150.0
];

#[rustfmt::skip]
pub(super) static  DEQUANT_IPD: [f32; N_IPD] = [
    0.00000000000000, 0.392699082, 0.78539816339745, 1.178097245,
    1.57079632679490, 1.963495408, 2.35619449019234, 2.748893572,
    3.14159265358979, 3.534291735, 3.92699081698724, 4.319689899,
    4.71238898038469, 5.105088062, 5.49778714378214, 5.890486225
];

pub(super) static SIN_IPD_TAB: [f32; N_IPD] = [
    0.000000000,
    0.382683456,
    0.707106769,
    0.923879504,
    1.000000000,
    0.923879564,
    0.707106769,
    0.382683486,
    -8.74227766e-08,
    -0.382683426,
    -0.707106709,
    -0.923879504,
    -1.000000000,
    -0.923879445,
    -0.707106888,
    -0.382683426,
];

pub(super) static SQRT_CLD_M: [f32; N_CLD] = [
    3.16227766016838e-008,
    0.00562341325190349,
    0.01,
    0.0177827941003892,
    0.0316227766016838,
    0.0562341325190349,
    0.0794328234724281,
    0.112201845430196,
    0.158489319246111,
    0.223872113856834,
    0.316227766016838,
    0.398107170553497,
    0.501187233627272,
    0.630957344480193,
    0.794328234724281,
    1.0,
    1.25892541179417,
    1.58489319246111,
    1.99526231496888,
    2.51188643150958,
    3.16227766016838,
    4.46683592150963,
    6.30957344480194,
    8.91250938133746,
    12.5892541179417,
    17.7827941003892,
    31.6227766016838,
    56.2341325190349,
    100.0,
    177.827941003892,
    31622776.6016838,
];

pub(super) static SAMPLING_FREQ_TABLE: [u32; 16] = [
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0,
    0,
];

pub(super) static FREQ_RES_TABLE: [FreqResolution; 8] = [
    FreqResolution::ResInvalid,
    FreqResolution::Res28,
    FreqResolution::Res20,
    FreqResolution::Res14,
    FreqResolution::Res10,
    FreqResolution::Res7,
    FreqResolution::Res5,
    FreqResolution::Res4,
];

pub(super) static TEMP_SHAPE_CHAN_TABLE: [u8; 2] = [2, 2];

pub(super) static FREQ_RES_TABLE_LD: [FreqResolution; 8] = [
    FreqResolution::ResInvalid,
    FreqResolution::Res23,
    FreqResolution::Res15,
    FreqResolution::Res12,
    FreqResolution::Res9,
    FreqResolution::Res7,
    FreqResolution::Res5,
    FreqResolution::Res4,
];

pub(super) static CLIP_GAIN_TABLE: [f32; 8] = [
    1.000000, 1.189207, 1.414213, 1.681792, 2.000000, 2.378414, 2.828427, 4.000000,
];
