
/* -----------------------------------------------------------------------------
Software Copyright License for The Fraunhofer FDK Extended High Efficiency AAC
Encoder Software for Android

© Copyright 1995 - 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1.    INTRODUCTION

The Fraunhofer FDK Extended High Efficiency AAC Encoder Software for Android
("FDK Extended High Efficiency AAC Encoder") is software that implements the
encoding of digital audio according to the MPEG-D Unified Speech and Audio
Coding (USAC) standard and MPEG-D Dynamic Range Control (DRC) standard. This FDK
Extended High Efficiency AAC Encoder Software is intended to be used on a wide
variety of Android devices. It is technically not suited to encode content for
digital radio broadcasting services, including DRM and similar standards.

Patent licenses for necessary patent claims for the FDK Extended High Efficiency
AAC Encoder Software (including those of Fraunhofer), for the use in commercial
products and services, may be obtained from the respective patent owners
individually and/or from Via Licensing Alliance (www.via-la.com).

Fraunhofer supports the development of Extended High Efficiency AAC products and
services by offering additional software, documentation, and technical advice.
In addition, it operates the xHE-AAC Trademark Program to ease interoperability
testing of end products. Please visit http://www.xhe-aac.com for more
information.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees, provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK Extended High
Efficiency AAC Encoder Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the
FDK Extended High Efficiency AAC Encoder Software and your modifications thereto
to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this software without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto.

Your modified versions of the FDK Extended High Efficiency AAC Encoder Software
must carry prominent notices stating that you changed the software and the date
of any change. For modified versions of the FDK Extended High Efficiency AAC
Encoder Software, the term "Fraunhofer FDK Extended High Efficiency AAC Encoder
Software for Android" must be replaced by the term "Third-Party Modified Version
of the Fraunhofer FDK Extended High Efficiency AAC Encoder Software for
Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty for patent non-infringement with respect to this
software. You may use this FDK Extended High Efficiency AAC Encoder Software or
modifications thereto only for purposes that are authorized by appropriate
patent licenses.

4.    DISCLAIMER

This FDK Extended High Efficiency AAC Encoder Software is provided by Fraunhofer
on behalf of the copyright holders and contributors "AS IS" and WITHOUT ANY
EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied
warranties of merchantability and fitness for a particular purpose. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if advised
of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - FDK Extended High Efficiency
AAC Encoder
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include "loudnessLib.h"
#include "version_loudnessLib.h"
#include "assert.h"

#ifdef __GNUC__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"
#endif

static const LOUDMTR_LIBINFO libInfo = {
    IIS_LOUDNESSLIB_VERSION,
    "Fraunhofer IIS Loudness Metering Library",
    "Copyright 2012-2023 Fraunhofer IIS",
    "Loudness metering implementation according to Recommendation ITU-R BS.1770-4 (Annex 3) and EBU Recommendation R 128"};

#define BLOCKLENGTH 100
#define MAXCHANNELS 56
#define UNDEFINED_LOUDNESS_VALUE (1000.f)

#define WEIGHT_M_FRONT (1.00f)
#define WEIGHT_M_SIDE (1.41f)
#define WEIGHT_M_BACK (1.00f)
#define WEIGHT_U_B_ALL (1.00f)
#define WEIGHT_LFE (0.00f)

#define PI 3.14159265f

#define MOMENT_LEN 4
#define SHORT_LEN 30
#define LR_STRIDE 10

#define GATE_THR_ABS -70
#define GATE_THR_REL -10
#define GATE_THR_REL_LR -20

#define HIST_RANGE 100
#define HIST_RES 10
#define HIST_SIZE (HIST_RANGE * HIST_RES)

#define PERC_LO 10
#define PERC_HI 95

#define TP_FILT_IIS_LEN 20
#define TP_FILT_ITU_LEN 12
#define TP_FILT_MAX_LEN TP_FILT_IIS_LEN

#define LOUD_MIN -150

#define LIN2DB(x) (((x) > 0) ? (20 * (float)log10(x)) : (LOUD_MIN))
#define NRG2LUFS(x) (((x) > 0) ? (10 * (float)log10(x) - 0.691f) : (LOUD_MIN))
#define LUFS2NRG(x) ((float)pow(10, ((x) + 0.691f) * 0.1f))
#if HIST_RANGE == 100 && HIST_RES == 10
#define HI2NRG(x) hi2Nrg_table[x]
#else
#define HI2NRG(x) ((float)pow(10, (((x) + 0.5f) / HIST_RES + GATE_THR_ABS + 0.691f) * 0.1f))
#endif

#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

#define EN_LIMIT (1.1724637e-015)

static const float tpFilt_IIS[4][TP_FILT_IIS_LEN] = {
    {0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
     1.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
    {-0.003906f, 0.006828f, -0.010843f, 0.016275f, -0.023640f, 0.033866f, -0.048870f, 0.073390f, -0.123109f, 0.297714f,
     0.899517f, -0.176098f, 0.093026f, -0.059415f, 0.040580f, -0.028310f, 0.019671f, -0.013356f, 0.008682f, -0.005246f},
    {-0.006431f, 0.010917f, -0.017045f, 0.025326f, -0.036596f, 0.052406f, -0.076089f, 0.116378f, -0.205502f, 0.634362f,
     0.634362f, -0.205502f, 0.116378f, -0.076089f, 0.052406f, -0.036596f, 0.025326f, -0.017045f, 0.010917f, -0.006431f},
    {-0.005246f, 0.008682f, -0.013356f, 0.019671f, -0.028310f, 0.040580f, -0.059415f, 0.093026f, -0.176098f, 0.899517f,
     0.297714f, -0.123109f, 0.073390f, -0.048870f, 0.033866f, -0.023640f, 0.016275f, -0.010843f, 0.006828f, -0.003906f}};

static const float tpFilt_ITU[4][TP_FILT_ITU_LEN] = {
    {0.0017089843750f, 0.0109863281250f, -0.0196533203125f, 0.0332031250000f, -0.0594482421875f, 0.1373291015625f,
     0.9721679687500f, -0.1022949218750f, 0.0476074218750f, -0.0266113281250f, 0.0148925781250f, -0.0083007812500f},
    {-0.0291748046875f, 0.0292968750000f, -0.0517578125000f, 0.0891113281250f, -0.1665039062500f, 0.4650878906250f,
     0.7797851562500f, -0.2003173828125f, 0.1015625000000f, -0.0582275390625f, 0.0330810546875f, -0.0189208984375f},
    {-0.0189208984375f, 0.0330810546875f, -0.0582275390625f, 0.1015625000000f, -0.2003173828125f, 0.7797851562500f,
     0.4650878906250f, -0.1665039062500f, 0.0891113281250f, -0.0517578125000f, 0.0292968750000f, -0.0291748046875f},
    {-0.0083007812500f, 0.0148925781250f, -0.0266113281250f, 0.0476074218750f, -0.1022949218750f, 0.9721679687500f,
     0.1373291015625f, -0.0594482421875f, 0.0332031250000f, -0.0196533203125f, 0.0109863281250f, 0.0017089843750f}};

#if HIST_RANGE == 100 && HIST_RES == 10
static const float hi2Nrg_table[HIST_SIZE] = {
    1.18604319e-07f, 1.21366895e-07f, 1.24193818e-07f, 1.27086594e-07f, 1.30046899e-07f, 1.33076142e-07f, 1.36175956e-07f, 1.39347819e-07f, 1.42593564e-07f, 1.45914910e-07f,
    1.49313948e-07f, 1.52791841e-07f, 1.56350723e-07f, 1.59992680e-07f, 1.63719307e-07f, 1.67533088e-07f, 1.71435332e-07f, 1.75428482e-07f, 1.79514629e-07f, 1.83695960e-07f,
    1.87975090e-07f, 1.92353482e-07f, 1.96834080e-07f, 2.01418814e-07f, 2.06110343e-07f, 2.10911608e-07f, 2.15824258e-07f, 2.20851319e-07f, 2.25995478e-07f, 2.31259705e-07f,
    2.36646571e-07f, 2.42158904e-07f, 2.47799363e-07f, 2.53571216e-07f, 2.59477503e-07f, 2.65521919e-07f, 2.71706568e-07f, 2.78035287e-07f, 2.84511714e-07f, 2.91138662e-07f,
    2.97920650e-07f, 3.04859924e-07f, 3.11960861e-07f, 3.19227155e-07f, 3.26662729e-07f, 3.34272244e-07f, 3.42058229e-07f, 3.50025971e-07f, 3.58178937e-07f, 3.66521789e-07f,
    3.75059784e-07f, 3.83795822e-07f, 3.92735359e-07f, 4.01883113e-07f, 4.11244372e-07f, 4.20823710e-07f, 4.30626187e-07f, 4.40656521e-07f, 4.50920453e-07f, 4.61423497e-07f,
    4.72171138e-07f, 4.83170197e-07f, 4.94424398e-07f, 5.05941273e-07f, 5.17725880e-07f, 5.29784984e-07f, 5.42126145e-07f, 5.54753569e-07f, 5.67675102e-07f, 5.80897620e-07f,
    5.94428798e-07f, 6.08275116e-07f, 6.22444020e-07f, 6.36942218e-07f, 6.51778123e-07f, 6.66959636e-07f, 6.82496193e-07f, 6.98393194e-07f, 7.14660473e-07f, 7.31307409e-07f,
    7.48341336e-07f, 7.65773677e-07f, 7.83610403e-07f, 8.01862598e-07f, 8.20539867e-07f, 8.39652216e-07f, 8.59211639e-07f, 8.79224729e-07f, 8.99705014e-07f, 9.20661307e-07f,
    9.42105714e-07f, 9.64051765e-07f, 9.86506848e-07f, 1.00948489e-06f, 1.03299828e-06f, 1.05706044e-06f, 1.08168308e-06f, 1.10687938e-06f, 1.13266128e-06f, 1.15904368e-06f,
    1.18604055e-06f, 1.21366895e-06f, 1.24193821e-06f, 1.27086594e-06f, 1.30046897e-06f, 1.33076003e-06f, 1.36175959e-06f, 1.39347821e-06f, 1.42593569e-06f, 1.45914919e-06f,
    1.49313632e-06f, 1.52791836e-06f, 1.56350723e-06f, 1.59992692e-06f, 1.63719301e-06f, 1.67532721e-06f, 1.71435340e-06f, 1.75428477e-06f, 1.79514632e-06f, 1.83695954e-06f,
    1.87974888e-06f, 1.92353491e-06f, 1.96834071e-06f, 2.01418811e-06f, 2.06110349e-06f, 2.10911139e-06f, 2.15824252e-06f, 2.20851325e-06f, 2.25995473e-06f, 2.31259719e-06f,
    2.36646315e-06f, 2.42158899e-06f, 2.47799358e-06f, 2.53571216e-06f, 2.59477497e-06f, 2.65521339e-06f, 2.71706585e-06f, 2.78035282e-06f, 2.84511702e-06f, 2.91138667e-06f,
    2.97919973e-06f, 3.04859918e-06f, 3.11960844e-06f, 3.19227161e-06f, 3.26662735e-06f, 3.34271863e-06f, 3.42058229e-06f, 3.50025994e-06f, 3.58178931e-06f, 3.66521772e-06f,
    3.75058949e-06f, 3.83795805e-06f, 3.92735365e-06f, 4.01883108e-06f, 4.11244400e-06f, 4.20823244e-06f, 4.30626187e-06f, 4.40656504e-06f, 4.50920470e-06f, 4.61423497e-06f,
    4.72171132e-06f, 4.83170197e-06f, 4.94424421e-06f, 5.05941307e-06f, 5.17725903e-06f, 5.29784984e-06f, 5.42126145e-06f, 5.54753569e-06f, 5.67675079e-06f, 5.80897631e-06f,
    5.94428775e-06f, 6.08275104e-06f, 6.22443986e-06f, 6.36942241e-06f, 6.51778146e-06f, 6.66959613e-06f, 6.82496193e-06f, 6.98393205e-06f, 7.14660473e-06f, 7.31307409e-06f,
    7.48341336e-06f, 7.65773711e-06f, 7.83610358e-06f, 8.01862552e-06f, 8.20539844e-06f, 8.39652239e-06f, 8.59211650e-06f, 8.79224717e-06f, 8.99704992e-06f, 9.20661296e-06f,
    9.42105726e-06f, 9.64051742e-06f, 9.86506802e-06f, 1.00948491e-05f, 1.03299826e-05f, 1.05706049e-05f, 1.08168315e-05f, 1.10687934e-05f, 1.13266124e-05f, 1.15904368e-05f,
    1.18604057e-05f, 1.21366893e-05f, 1.24193821e-05f, 1.27086596e-05f, 1.30046892e-05f, 1.33076001e-05f, 1.36175959e-05f, 1.39347821e-05f, 1.42593572e-05f, 1.45914919e-05f,
    1.49313628e-05f, 1.52791836e-05f, 1.56350725e-05f, 1.59992687e-05f, 1.63719305e-05f, 1.67532726e-05f, 1.71435331e-05f, 1.75428486e-05f, 1.79514627e-05f, 1.83695956e-05f,
    1.87974892e-05f, 1.92353491e-05f, 1.96834080e-05f, 2.01418807e-05f, 2.06110344e-05f, 2.10911148e-05f, 2.15824257e-05f, 2.20851325e-05f, 2.25995482e-05f, 2.31259710e-05f,
    2.36646301e-05f, 2.42158894e-05f, 2.47799362e-05f, 2.53571216e-05f, 2.59477492e-05f, 2.65521358e-05f, 2.71706576e-05f, 2.78035277e-05f, 2.84511698e-05f, 2.91138658e-05f,
    2.97919978e-05f, 3.04859932e-05f, 3.11960866e-05f, 3.19227183e-05f, 3.26662739e-05f, 3.34271863e-05f, 3.42058229e-05f, 3.50025985e-05f, 3.58178950e-05f, 3.66521781e-05f,
    3.75058953e-05f, 3.83795814e-05f, 3.92735346e-05f, 4.01883117e-05f, 4.11244400e-05f, 4.20823271e-05f, 4.30626205e-05f, 4.40656513e-05f, 4.50920452e-05f, 4.61423479e-05f,
    4.72171159e-05f, 4.83170224e-05f, 4.94424421e-05f, 5.05941316e-05f, 5.17725894e-05f, 5.29784957e-05f, 5.42126145e-05f, 5.54753569e-05f, 5.67675088e-05f, 5.80897613e-05f,
    5.94428784e-05f, 6.08275113e-05f, 6.22443986e-05f, 6.36942204e-05f, 6.51778173e-05f, 6.66959604e-05f, 6.82496247e-05f, 6.98393196e-05f, 7.14660418e-05f, 7.31307446e-05f,
    7.48341336e-05f, 7.65773657e-05f, 7.83610376e-05f, 8.01862552e-05f, 8.20539863e-05f, 8.39652203e-05f, 8.59211650e-05f, 8.79224754e-05f, 8.99704974e-05f, 9.20661259e-05f,
    9.42105689e-05f, 9.64051724e-05f, 9.86506857e-05f, 1.00948550e-04f, 1.03299884e-04f, 1.05705985e-04f, 1.08168373e-04f, 1.10687877e-04f, 1.13266120e-04f, 1.15904368e-04f,
    1.18604119e-04f, 1.21366895e-04f, 1.24193888e-04f, 1.27086663e-04f, 1.30046828e-04f, 1.33075999e-04f, 1.36175877e-04f, 1.39347831e-04f, 1.42593562e-04f, 1.45914993e-04f,
    1.49313710e-04f, 1.52791923e-04f, 1.56350812e-04f, 1.59992604e-04f, 1.63719305e-04f, 1.67532722e-04f, 1.71435342e-04f, 1.75428475e-04f, 1.79514725e-04f, 1.83696058e-04f,
    1.87974787e-04f, 1.92353589e-04f, 1.96833964e-04f, 2.01418821e-04f, 2.06110344e-04f, 2.10911268e-04f, 2.15824257e-04f, 2.20851434e-04f, 2.25995609e-04f, 2.31259590e-04f,
    2.36646301e-04f, 2.42158771e-04f, 2.47799355e-04f, 2.53571197e-04f, 2.59477645e-04f, 2.65521492e-04f, 2.71706725e-04f, 2.78035441e-04f, 2.84511538e-04f, 2.91138655e-04f,
    2.97919993e-04f, 3.04859917e-04f, 3.11960845e-04f, 3.19227343e-04f, 3.26662906e-04f, 3.34271695e-04f, 3.42058425e-04f, 3.50025803e-04f, 3.58178921e-04f, 3.66521796e-04f,
    3.75059171e-04f, 3.83795821e-04f, 3.92735557e-04f, 4.01883328e-04f, 4.11244167e-04f, 4.20823257e-04f, 4.30625951e-04f, 4.40656499e-04f, 4.50920459e-04f, 4.61423740e-04f,
    4.72171407e-04f, 4.83170472e-04f, 4.94424661e-04f, 5.05941047e-04f, 5.17725886e-04f, 5.29785000e-04f, 5.42126130e-04f, 5.54753584e-04f, 5.67675394e-04f, 5.80897962e-04f,
    5.94428449e-04f, 6.08275470e-04f, 6.22443680e-04f, 6.36941870e-04f, 6.51778479e-04f, 6.66959968e-04f, 6.82495825e-04f, 6.98393211e-04f, 7.14660448e-04f, 7.31307431e-04f,
    7.48341728e-04f, 7.65773235e-04f, 7.83609983e-04f, 8.01862101e-04f, 8.20540357e-04f, 8.39653134e-04f, 8.59211665e-04f, 8.79224739e-04f, 8.99704522e-04f, 9.20661783e-04f,
    9.42106242e-04f, 9.64051229e-04f, 9.86506231e-04f, 1.00948499e-03f, 1.03299937e-03f, 1.05706044e-03f, 1.08168309e-03f, 1.10687874e-03f, 1.13266066e-03f, 1.15904433e-03f,
    1.18604116e-03f, 1.21366826e-03f, 1.24193821e-03f, 1.27086602e-03f, 1.30046892e-03f, 1.33076066e-03f, 1.36175880e-03f, 1.39347743e-03f, 1.42593496e-03f, 1.45914999e-03f,
    1.49313791e-03f, 1.52791839e-03f, 1.56350725e-03f, 1.59992604e-03f, 1.63719396e-03f, 1.67532812e-03f, 1.71435240e-03f, 1.75428379e-03f, 1.79514627e-03f, 1.83696160e-03f,
    1.87974889e-03f, 1.92353479e-03f, 1.96833978e-03f, 2.01418693e-03f, 2.06110463e-03f, 2.10911268e-03f, 2.15824135e-03f, 2.20851321e-03f, 2.25995481e-03f, 2.31259712e-03f,
    2.36646435e-03f, 2.42158771e-03f, 2.47799233e-03f, 2.53571081e-03f, 2.59477645e-03f, 2.65521649e-03f, 2.71706586e-03f, 2.78035272e-03f, 2.84511549e-03f, 2.91138818e-03f,
    2.97920150e-03f, 3.04859760e-03f, 3.11960676e-03f, 3.19227157e-03f, 3.26663093e-03f, 3.34271858e-03f, 3.42058251e-03f, 3.50025785e-03f, 3.58178746e-03f, 3.66521976e-03f,
    3.75059154e-03f, 3.83795612e-03f, 3.92735377e-03f, 4.01883107e-03f, 4.11244389e-03f, 4.20823507e-03f, 4.30625957e-03f, 4.40656254e-03f, 4.50920220e-03f, 4.61423723e-03f,
    4.72171651e-03f, 4.83170198e-03f, 4.94424393e-03f, 5.05941035e-03f, 5.17726177e-03f, 5.29785268e-03f, 5.42125851e-03f, 5.54753235e-03f, 5.67675103e-03f, 5.80898253e-03f,
    5.94428787e-03f, 6.08275132e-03f, 6.22443669e-03f, 6.36941846e-03f, 6.51778514e-03f, 6.66960003e-03f, 6.82495860e-03f, 6.98393211e-03f, 7.14660436e-03f, 7.31307408e-03f,
    7.48341763e-03f, 7.65773281e-03f, 7.83609971e-03f, 8.01862124e-03f, 8.20540357e-03f, 8.39653146e-03f, 8.59211665e-03f, 8.79224762e-03f, 8.99704546e-03f, 9.20661818e-03f,
    9.42106266e-03f, 9.64051206e-03f, 9.86506324e-03f, 1.00948494e-02f, 1.03299944e-02f, 1.05706071e-02f, 1.08168339e-02f, 1.10687846e-02f, 1.13266064e-02f, 1.15904426e-02f,
    1.18604153e-02f, 1.21366866e-02f, 1.24193821e-02f, 1.27086630e-02f, 1.30046932e-02f, 1.33076031e-02f, 1.36175845e-02f, 1.39347743e-02f, 1.42593533e-02f, 1.45915039e-02f,
    1.49313789e-02f, 1.52791841e-02f, 1.56350769e-02f, 1.59992557e-02f, 1.63719356e-02f, 1.67532805e-02f, 1.71435233e-02f, 1.75428428e-02f, 1.79514624e-02f, 1.83696151e-02f,
    1.87974945e-02f, 1.92353539e-02f, 1.96833909e-02f, 2.01418698e-02f, 2.06110459e-02f, 2.10911315e-02f, 2.15824191e-02f, 2.20851321e-02f, 2.25995537e-02f, 2.31259782e-02f,
    2.36646365e-02f, 2.42158696e-02f, 2.47799233e-02f, 2.53571142e-02f, 2.59477701e-02f, 2.65521649e-02f, 2.71706581e-02f, 2.78035346e-02f, 2.84511466e-02f, 2.91138738e-02f,
    2.97920145e-02f, 3.04859765e-02f, 3.11960764e-02f, 3.19227166e-02f, 3.26663107e-02f, 3.34271975e-02f, 3.42058316e-02f, 3.50025706e-02f, 3.58178727e-02f, 3.66521999e-02f,
    3.75059247e-02f, 3.83795723e-02f, 3.92735340e-02f, 4.01883200e-02f, 4.11244482e-02f, 4.20823358e-02f, 4.30625826e-02f, 4.40656282e-02f, 4.50920351e-02f, 4.61423881e-02f,
    4.72171679e-02f, 4.83170226e-02f, 4.94424552e-02f, 5.05940877e-02f, 5.17726019e-02f, 5.29785268e-02f, 5.42125851e-02f, 5.54753393e-02f, 5.67675084e-02f, 5.80898263e-02f,
    5.94428927e-02f, 6.08275309e-02f, 6.22443482e-02f, 6.36941865e-02f, 6.51778504e-02f, 6.66960180e-02f, 6.82496056e-02f, 6.98393211e-02f, 7.14660659e-02f, 7.31307641e-02f,
    7.48341531e-02f, 7.65773058e-02f, 7.83609971e-02f, 8.01862329e-02f, 8.20540562e-02f, 8.39653164e-02f, 8.59211609e-02f, 8.79224986e-02f, 8.99704248e-02f, 9.20661539e-02f,
    9.42106247e-02f, 9.64051187e-02f, 9.86506566e-02f, 1.00948498e-01f, 1.03299938e-01f, 1.05706058e-01f, 1.08168326e-01f, 1.10687859e-01f, 1.13266073e-01f, 1.15904443e-01f,
    1.18604153e-01f, 1.21366866e-01f, 1.24193825e-01f, 1.27086610e-01f, 1.30046904e-01f, 1.33076057e-01f, 1.36175871e-01f, 1.39347762e-01f, 1.42593533e-01f, 1.45915031e-01f,
    1.49313793e-01f, 1.52791843e-01f, 1.56350747e-01f, 1.59992576e-01f, 1.63719371e-01f, 1.67532831e-01f, 1.71435267e-01f, 1.75428435e-01f, 1.79514632e-01f, 1.83696166e-01f,
    1.87974915e-01f, 1.92353517e-01f, 1.96833938e-01f, 2.01418728e-01f, 2.06110492e-01f, 2.10911319e-01f, 2.15824187e-01f, 2.20851317e-01f, 2.25995511e-01f, 2.31259748e-01f,
    2.36646399e-01f, 2.42158741e-01f, 2.47799262e-01f, 2.53571153e-01f, 2.59477705e-01f, 2.65521646e-01f, 2.71706581e-01f, 2.78035313e-01f, 2.84511507e-01f, 2.91138798e-01f,
    2.97920436e-01f, 3.04859787e-01f, 3.11960757e-01f, 3.19227159e-01f, 3.26662809e-01f, 3.34272236e-01f, 3.42058301e-01f, 3.50025743e-01f, 3.58178765e-01f, 3.66521716e-01f,
    3.75059605e-01f, 3.83795738e-01f, 3.92735362e-01f, 4.01883185e-01f, 4.11244094e-01f, 4.20823812e-01f, 4.30625886e-01f, 4.40656304e-01f, 4.50920373e-01f, 4.61423486e-01f,
    4.72172081e-01f, 4.83170211e-01f, 4.94424522e-01f, 5.05940974e-01f, 5.17725646e-01f, 5.29785752e-01f, 5.42125881e-01f, 5.54753423e-01f, 5.67675114e-01f, 5.80897748e-01f,
    5.94429433e-01f, 6.08275235e-01f, 6.22443616e-01f, 6.36941910e-01f, 6.51777983e-01f, 6.66960776e-01f, 6.82496071e-01f, 6.98393166e-01f, 7.14660585e-01f, 7.31306970e-01f,
    7.48342335e-01f, 7.65773177e-01f, 7.83610046e-01f, 8.01862419e-01f, 8.20539892e-01f, 8.39653909e-01f, 8.59211624e-01f, 8.79224956e-01f, 8.99704397e-01f, 9.20660913e-01f,
    9.42107141e-01f, 9.64051306e-01f, 9.86506581e-01f, 1.00948489e+00f, 1.03299856e+00f, 1.05706167e+00f, 1.08168340e+00f, 1.10687864e+00f, 1.13266075e+00f, 1.15904343e+00f,
    1.18604267e+00f, 1.21366870e+00f, 1.24193823e+00f, 1.27086627e+00f, 1.30046809e+00f, 1.33076179e+00f, 1.36175871e+00f, 1.39347756e+00f, 1.42593539e+00f, 1.45914912e+00f,
    1.49313927e+00f, 1.52791834e+00f, 1.56350768e+00f, 1.59992588e+00f, 1.63719225e+00f, 1.67532980e+00f, 1.71435261e+00f, 1.75428438e+00f, 1.79514635e+00f, 1.83695996e+00f,
    1.87975097e+00f, 1.92353523e+00f, 1.96833956e+00f, 2.01418734e+00f, 2.06110311e+00f, 2.10911536e+00f, 2.15824199e+00f, 2.20851326e+00f, 2.25995517e+00f, 2.31259561e+00f,
    2.36646605e+00f, 2.42158747e+00f, 2.47799253e+00f, 2.53571153e+00f, 2.59477496e+00f, 2.65521884e+00f, 2.71706581e+00f, 2.78035331e+00f, 2.84511518e+00f, 2.91138554e+00f,
    2.97920465e+00f, 3.04859805e+00f, 3.11960793e+00f, 3.19227171e+00f, 3.26662827e+00f, 3.34272242e+00f, 3.42058325e+00f, 3.50025749e+00f, 3.58178782e+00f, 3.66521740e+00f,
    3.75059628e+00f, 3.83795762e+00f, 3.92735362e+00f, 4.01883221e+00f, 4.11244106e+00f, 4.20823812e+00f, 4.30625916e+00f, 4.40656328e+00f, 4.50920391e+00f, 4.61423492e+00f,
    4.72172117e+00f, 4.83170223e+00f, 4.94424534e+00f, 5.05940962e+00f, 5.17725658e+00f, 5.29785776e+00f, 5.42125893e+00f, 5.54753494e+00f, 5.67675114e+00f, 5.80897760e+00f,
    5.94429445e+00f, 6.08275270e+00f, 6.22443581e+00f, 6.36941957e+00f, 6.51778030e+00f, 6.66960812e+00f, 6.82496119e+00f, 6.98393202e+00f, 7.14660645e+00f, 7.31306934e+00f,
    7.48342371e+00f, 7.65773153e+00f, 7.83610058e+00f, 8.01862431e+00f, 8.20539856e+00f, 8.39653969e+00f, 8.59211636e+00f, 8.79224968e+00f, 8.99704361e+00f, 9.20660877e+00f,
    9.42107105e+00f, 9.64051342e+00f, 9.86506653e+00f, 1.00948496e+01f, 1.03299856e+01f, 1.05706158e+01f, 1.08168344e+01f, 1.10687876e+01f, 1.13266096e+01f, 1.15904331e+01f,
    1.18604288e+01f, 1.21366863e+01f, 1.24193821e+01f, 1.27086630e+01f, 1.30046825e+01f, 1.33076181e+01f, 1.36175880e+01f, 1.39347782e+01f, 1.42593527e+01f, 1.45914917e+01f,
    1.49313917e+01f, 1.52791834e+01f, 1.56350765e+01f, 1.59992599e+01f, 1.63719254e+01f, 1.67532997e+01f, 1.71435280e+01f, 1.75428429e+01f, 1.79514637e+01f, 1.83696003e+01f,
    1.87975082e+01f, 1.92353535e+01f, 1.96833973e+01f, 2.01418762e+01f, 2.06110287e+01f, 2.10911560e+01f, 2.15824184e+01f, 2.20851326e+01f, 2.25995541e+01f, 2.31259594e+01f,
    2.36646633e+01f, 2.42158775e+01f, 2.47799301e+01f, 2.53571148e+01f, 2.59477501e+01f, 2.65521851e+01f, 2.71706581e+01f, 2.78035355e+01f, 2.84511547e+01f, 2.91138592e+01f,
    2.97920475e+01f, 3.04859848e+01f, 3.11960754e+01f, 3.19227161e+01f, 3.26662827e+01f, 3.34272232e+01f, 3.42058334e+01f, 3.50025787e+01f, 3.58178825e+01f, 3.66521683e+01f,
    3.75059662e+01f, 3.83795738e+01f, 3.92735367e+01f, 4.01883202e+01f, 4.11244164e+01f, 4.20823822e+01f, 4.30625954e+01f, 4.40656395e+01f, 4.50920334e+01f, 4.61423492e+01f,
    4.72172050e+01f, 4.83170204e+01f, 4.94424553e+01f, 5.05941010e+01f, 5.17725754e+01f, 5.29785843e+01f, 5.42125969e+01f, 5.54753418e+01f, 5.67675095e+01f, 5.80897789e+01f,
    5.94429436e+01f, 6.08275299e+01f, 6.22443657e+01f, 6.36942062e+01f, 6.51777954e+01f, 6.66960907e+01f, 6.82496033e+01f, 6.98393173e+01f, 7.14660645e+01f, 7.31306992e+01f,
    7.48342361e+01f, 7.65773239e+01f, 7.83610153e+01f, 8.01862335e+01f, 8.20539856e+01f, 8.39653854e+01f, 8.59211655e+01f, 8.79225006e+01f, 8.99704514e+01f, 9.20661011e+01f,
    9.42107239e+01f, 9.64051437e+01f, 9.86506577e+01f, 1.00948494e+02f, 1.03299881e+02f, 1.05706161e+02f, 1.08168373e+02f, 1.10687874e+02f, 1.13266060e+02f, 1.15904366e+02f,
    1.18604248e+02f, 1.21366898e+02f, 1.24193825e+02f, 1.27086670e+02f, 1.30046814e+02f, 1.33076218e+02f, 1.36175888e+02f, 1.39347748e+02f, 1.42593567e+02f, 1.45914917e+02f,
    1.49313950e+02f, 1.52791840e+02f, 1.56350815e+02f, 1.59992599e+02f, 1.63719208e+02f, 1.67532990e+02f, 1.71435242e+02f, 1.75428482e+02f, 1.79514633e+02f, 1.83696060e+02f,
    1.87975098e+02f, 1.92353592e+02f, 1.96833969e+02f, 2.01418701e+02f, 2.06110352e+02f, 2.10911499e+02f, 2.15824249e+02f, 2.20851318e+02f, 2.25995605e+02f, 2.31259583e+02f,
    2.36646698e+02f, 2.42158768e+02f, 2.47799225e+02f, 2.53571213e+02f, 2.59477478e+02f, 2.65521942e+02f, 2.71706573e+02f, 2.78035431e+02f, 2.84511536e+02f, 2.91138489e+02f,
    2.97920471e+02f, 3.04859772e+02f, 3.11960846e+02f, 3.19227173e+02f, 3.26662903e+02f, 3.34272247e+02f, 3.42058441e+02f, 3.50025787e+02f, 3.58178741e+02f, 3.66521790e+02f,
    3.75059570e+02f, 3.83795837e+02f, 3.92735352e+02f, 4.01883331e+02f, 4.11244171e+02f, 4.20823944e+02f, 4.30625946e+02f, 4.40656281e+02f, 4.50920471e+02f, 4.61423492e+02f,
    4.72172180e+02f, 4.83170197e+02f, 4.94424683e+02f, 5.05941010e+02f, 5.17725586e+02f, 5.29785828e+02f, 5.42125854e+02f, 5.54753540e+02f, 5.67675110e+02f, 5.80897949e+02f,
    5.94429443e+02f, 6.08275452e+02f, 6.22443665e+02f, 6.36941895e+02f, 6.51778137e+02f, 6.66960693e+02f, 6.82496216e+02f, 6.98393188e+02f, 7.14660828e+02f, 7.31307007e+02f,
    7.48342590e+02f, 7.65773254e+02f, 7.83609985e+02f, 8.01862549e+02f, 8.20539856e+02f, 8.39654053e+02f, 8.59211609e+02f, 8.79225220e+02f, 8.99704529e+02f, 9.20660767e+02f,
    9.42107239e+02f, 9.64051208e+02f, 9.86506836e+02f, 1.00948492e+03f, 1.03299890e+03f, 1.05706165e+03f, 1.08168372e+03f, 1.10687878e+03f, 1.13266064e+03f, 1.15904370e+03f};
#endif

typedef struct BIQUAD_COEFF {
  float b0, b1, b2;
  float a1, a2;
} BIQUAD_COEFF;

typedef struct BIQUAD_STATE {
  float z1, z2;
} BIQUAD_STATE;

typedef struct LOUDNESS_METER {
  unsigned int nCh;
  unsigned int fs;
  float chWeights[MAXCHANNELS];
  unsigned int channelGroup[MAXCHANNELS];
  unsigned int blockLen, blockPos;
  float chNrg[MAXCHANNELS];
  float blockTP[MAXCHANNELS];
  float blockSP[MAXCHANNELS];
  LOUDMTR_WORKLOAD statusWorkload;

  struct {
    BIQUAD_COEFF coeff;
    BIQUAD_STATE state[MAXCHANNELS];
  } preFilt, rlbFilt;

  LOUDMTR_TPFILTER filtCfg;
  float tpBuf[TP_FILT_MAX_LEN * MAXCHANNELS];
  unsigned int tpBufPos;

  float momentBufNrg[MOMENT_LEN], shortBufNrg[SHORT_LEN];
  float momentBufTP[MOMENT_LEN][MAXCHANNELS];
  float momentBufSP[MOMENT_LEN][MAXCHANNELS];
  float shortBufTP[SHORT_LEN][MAXCHANNELS];
  float shortBufSP[SHORT_LEN][MAXCHANNELS];
  int momentBufPos, shortBufPos;

  unsigned long long momentHist[HIST_SIZE];
  unsigned long long shortHist[HIST_SIZE];

  float ungatedNrg, absgatedNrg, absgatedNrgLR;
  unsigned long long ungatedCnt, absgatedCnt, absgatedCntLR;

  float maxTruePeak[MAXCHANNELS];
  float maxSamplePeak[MAXCHANNELS];

  float maxMomentNrg, maxShortNrg;

  float gateThrAbs;

  float *momentaryEnergy;
  unsigned int momentaryEnergyLength;
  unsigned int momentaryEnergyIndex;
  int bGatedMidTermLoudnessDirty;
  float gatedMidTermLoudness;
  unsigned int gatedMidTermLoudnessCount;

  short *hiDelayLine;
  unsigned int hiDelayLineLength;
  unsigned int hiDelayLineIndex;
  unsigned long long *midTermShortHist;
  int bMidTermLRADirty;
  float midTermLRA;
  float midTermLRALow;
  float midTermLRAHigh;
  unsigned int midTermLRACount;

} LOUDNESS_METER;

static void setKweightFilt(BIQUAD_COEFF *preFilt, BIQUAD_COEFF *rlbFilt, unsigned int fs);

static int NRG2HI(float nrg);

static LOUDMTR_ERROR s_CalculateLoudness(const float *momentaryLoudness,
                                         const unsigned int momentaryLoudnessLength,
                                         float *gatedLoudnessOut,
                                         unsigned int *gatedCountOut);

static float s_getLRAFromHistogram(unsigned long long *shortHist,
                                   float absGatedNrgLR,
                                   unsigned long long absGatedCntLR,
                                   float *pLow,
                                   float *pHigh);

static float s_CalculateLRA(unsigned long long *midTermShortHist, const float gateThrAbs, unsigned int *lraCount, float *pLow, float *pHigh);

LOUDMTR_ERROR LoudMeter_Create(HLOUDNESS_METER *phLoudnessMeter) {
  if (!phLoudnessMeter) return ERRLOUD_InvalidArg;

  *phLoudnessMeter = (HLOUDNESS_METER)calloc(1, sizeof(LOUDNESS_METER));
  if (!*phLoudnessMeter) return ERRLOUD_OutOfMemory;

  (*phLoudnessMeter)->gateThrAbs = LUFS2NRG(GATE_THR_ABS);

  (*phLoudnessMeter)->statusWorkload = LOUDMTR_WORKLOAD_ENABLE_ALL;

  return ERRLOUD_Success;
}

LOUDMTR_ERROR LoudMeter_Destroy(HLOUDNESS_METER *phLoudnessMeter) {
  if (!phLoudnessMeter) return ERRLOUD_InvalidArg;
  if (*phLoudnessMeter == NULL) return ERRLOUD_Success;

  if ((*phLoudnessMeter)->midTermShortHist) {
    free((*phLoudnessMeter)->midTermShortHist);
  }

  if ((*phLoudnessMeter)->hiDelayLine) {
    free((*phLoudnessMeter)->hiDelayLine);
  }

  if ((*phLoudnessMeter)->momentaryEnergy) {
    free((*phLoudnessMeter)->momentaryEnergy);
  }

  free(*phLoudnessMeter);

  *phLoudnessMeter = NULL;

  return ERRLOUD_Success;
}

LOUDMTR_ERROR LoudMeter_Config(HLOUDNESS_METER hLoudnessMeter, LOUDMTR_LS_POS *channelConfig, int *channelGroup, int nCh, unsigned int nSamplesPerSec, LOUDMTR_TPFILTER filterConfig) {
  int i;
  float channelGroupFactor;

  if (!hLoudnessMeter) return ERRLOUD_InvalidArg;
  if (!nSamplesPerSec) return ERRLOUD_InvalidArg;
  if (!channelConfig) return ERRLOUD_InvalidArg;
  if (!channelGroup) return ERRLOUD_InvalidArg;
  if (nCh > MAXCHANNELS) return ERRLOUD_InvalidArg;

  hLoudnessMeter->nCh = nCh;

  i = 0;
  while (i < nCh) {
    if (channelGroup[i] == 0)
      channelGroupFactor = 0.f;
    else if (channelGroup[i] == 1)
      channelGroupFactor = 1.f;
    else

      return ERRLOUD_InvalidArg;

    if (channelConfig[i] == LS_POS_LFE)
      hLoudnessMeter->chWeights[i] = WEIGHT_LFE * channelGroupFactor;

    else if (channelConfig[i] == LS_POS_M_FRONT)
      hLoudnessMeter->chWeights[i] = WEIGHT_M_FRONT * channelGroupFactor;

    else if (channelConfig[i] == LS_POS_M_SIDE)
      hLoudnessMeter->chWeights[i] = WEIGHT_M_SIDE * channelGroupFactor;

    else if (channelConfig[i] == LS_POS_M_BACK)
      hLoudnessMeter->chWeights[i] = WEIGHT_M_BACK * channelGroupFactor;

    else if (channelConfig[i] == LS_POS_U_B_ALL)
      hLoudnessMeter->chWeights[i] = WEIGHT_U_B_ALL * channelGroupFactor;
    else {
      return ERRLOUD_InvalidArg;
    }

    hLoudnessMeter->channelGroup[i] = channelGroup[i];

    i++;
  }

  hLoudnessMeter->filtCfg = filterConfig;

  if (nSamplesPerSec != hLoudnessMeter->fs) {
    setKweightFilt(&(hLoudnessMeter->preFilt.coeff), &(hLoudnessMeter->rlbFilt.coeff), nSamplesPerSec);

    hLoudnessMeter->blockLen = BLOCKLENGTH * nSamplesPerSec / 1000;
    hLoudnessMeter->fs = nSamplesPerSec;
  }

  LoudMeter_Reset(hLoudnessMeter);

  return ERRLOUD_Success;
}

LOUDMTR_ERROR LoudMeter_Reset(HLOUDNESS_METER hLoudnessMeter) {
  unsigned int i, c;

  if (!hLoudnessMeter) return ERRLOUD_InvalidArg;

  for (i = 0; i < hLoudnessMeter->nCh; i++) {
    hLoudnessMeter->preFilt.state[i].z1 = hLoudnessMeter->preFilt.state[i].z2 = 0;
  }

  hLoudnessMeter->blockPos = 0;
  for (i = 0; i < MAXCHANNELS; i++) hLoudnessMeter->chNrg[i] = 0.f;
  for (i = 0; i < MAXCHANNELS; i++) hLoudnessMeter->blockTP[i] = hLoudnessMeter->blockSP[i] = 0;

  hLoudnessMeter->momentBufPos = hLoudnessMeter->shortBufPos = 0;

  for (i = 0; i < MOMENT_LEN; i++) {
    hLoudnessMeter->momentBufNrg[i] = 0;
    for (c = 0; c < MAXCHANNELS; c++) hLoudnessMeter->momentBufTP[i][c] = hLoudnessMeter->momentBufSP[i][c] = 0;
  }

  for (i = 0; i < SHORT_LEN; i++) {
    hLoudnessMeter->shortBufNrg[i] = 0;
    for (c = 0; c < MAXCHANNELS; c++) hLoudnessMeter->shortBufTP[i][c] = hLoudnessMeter->shortBufSP[i][c] = 0;
  }

  hLoudnessMeter->ungatedCnt = hLoudnessMeter->absgatedCnt = hLoudnessMeter->absgatedCntLR = 0;
  hLoudnessMeter->ungatedNrg = hLoudnessMeter->absgatedNrg = hLoudnessMeter->absgatedNrgLR = 0;

  for (i = 0; i < HIST_SIZE; i++) {
    hLoudnessMeter->momentHist[i] = 0;
    hLoudnessMeter->shortHist[i] = 0;
  }

  for (i = 0; i < MAXCHANNELS; i++) hLoudnessMeter->maxTruePeak[i] = hLoudnessMeter->maxSamplePeak[i] = 0;
  for (i = 0; i < TP_FILT_MAX_LEN * MAXCHANNELS; i++) hLoudnessMeter->tpBuf[i] = 0;
  hLoudnessMeter->tpBufPos = 0;

  hLoudnessMeter->maxMomentNrg = 0;
  hLoudnessMeter->maxShortNrg = 0;

  hLoudnessMeter->momentaryEnergyIndex = 0;
  hLoudnessMeter->bGatedMidTermLoudnessDirty = 1;
  for (i = 0; i < hLoudnessMeter->momentaryEnergyLength; i++) {
    hLoudnessMeter->momentaryEnergy[i] = 0;
  }

  hLoudnessMeter->hiDelayLineIndex = 0;
  hLoudnessMeter->bMidTermLRADirty = 1;
  for (i = 0; i < hLoudnessMeter->hiDelayLineLength; i++) {
    hLoudnessMeter->hiDelayLine[i] = -1;
  }

  return ERRLOUD_Success;
}

LOUDMTR_ERROR LoudMeter_Feed(HLOUDNESS_METER hLoudnessMeter, const float *pAudio, unsigned int nSamplesPerCh) {
  const unsigned int nCh = hLoudnessMeter->nCh;
  const unsigned int blockLen = hLoudnessMeter->blockLen;
  const BIQUAD_COEFF preC = hLoudnessMeter->preFilt.coeff, rlbC = hLoudnessMeter->rlbFilt.coeff;
  float *const tpBuf = hLoudnessMeter->tpBuf;
  float *const blockTP = hLoudnessMeter->blockTP;
  float *const blockSP = hLoudnessMeter->blockSP;

  unsigned int blockPos = hLoudnessMeter->blockPos;
  unsigned int tpBufPos = hLoudnessMeter->tpBufPos;

  unsigned int n, c, i, j, nSamplesLeft;
  unsigned int tpFiltLen = 0;

  if (!hLoudnessMeter || !pAudio) return ERRLOUD_InvalidArg;

  if (hLoudnessMeter->filtCfg == LOUDMTR_TPFILTER_IIS) {
    tpFiltLen = TP_FILT_IIS_LEN;
  } else {
    tpFiltLen = TP_FILT_ITU_LEN;
  }

  nSamplesLeft = nSamplesPerCh;
  while (nSamplesLeft) {
    n = blockLen - blockPos;
    if (n > nSamplesLeft) n = nSamplesLeft;

    if (!(hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS)) {
      for (c = 0; c < nCh; c++) {
        float chNrg = hLoudnessMeter->chNrg[c];
        BIQUAD_STATE preS = hLoudnessMeter->preFilt.state[c];
        BIQUAD_STATE rlbS = hLoudnessMeter->rlbFilt.state[c];

        if (hLoudnessMeter->chWeights[c] == 0.0f) continue;

        for (i = 0; i < n; i++) {
          float tmp = pAudio[i * nCh + c], z0;

          z0 = tmp - preC.a1 * preS.z1 - preC.a2 * preS.z2;
          tmp = preC.b0 * z0 + preC.b1 * preS.z1 + preC.b2 * preS.z2;
          preS.z2 = preS.z1;
          preS.z1 = z0;

          z0 = tmp - rlbC.a1 * rlbS.z1 - rlbC.a2 * rlbS.z2;
          tmp = rlbC.b0 * z0 + rlbC.b1 * rlbS.z1 + rlbC.b2 * rlbS.z2;
          rlbS.z2 = rlbS.z1;
          rlbS.z1 = z0;

          chNrg += tmp * tmp;
        }

        hLoudnessMeter->preFilt.state[c] = preS;
        hLoudnessMeter->rlbFilt.state[c] = rlbS;

        hLoudnessMeter->chNrg[c] = chNrg;
      }
    }
    if (!(hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_TP)) {
      for (c = 0; c < nCh; c++) {
        unsigned int tmpBufPos = tpBufPos;

        if (hLoudnessMeter->channelGroup[c] == 0) continue;

        for (i = 0; i < n; i++) {
          float s0 = 0, s1 = 0, s2 = 0, s3 = 0;

          tpBuf[tmpBufPos * nCh + c] = pAudio[i * nCh + c];

          if (hLoudnessMeter->filtCfg == LOUDMTR_TPFILTER_IIS) {
            unsigned int k0 = (tmpBufPos + tpFiltLen / 2) % tpFiltLen;
            s0 = tpBuf[k0 * nCh + c];

            for (j = 0; j < tpFiltLen; j++) {
              unsigned int k = (tmpBufPos + tpFiltLen - j) % tpFiltLen;
              float s = tpBuf[k * nCh + c];

              s1 += tpFilt_IIS[1][j] * s;
              s2 += tpFilt_IIS[2][j] * s;
              s3 += tpFilt_IIS[3][j] * s;
            }
          } else {
            for (j = 0; j < tpFiltLen; j++) {
              unsigned int k = (tmpBufPos + tpFiltLen - j) % tpFiltLen;
              float s = tpBuf[k * nCh + c];

              s0 += tpFilt_ITU[0][j] * s;
              s1 += tpFilt_ITU[1][j] * s;
              s2 += tpFilt_ITU[2][j] * s;
              s3 += tpFilt_ITU[3][j] * s;
            }
          }

          s0 = (float)fabs(s0);
          s1 = (float)fabs(s1);
          s2 = (float)fabs(s2);
          s3 = (float)fabs(s3);

          if (s0 > blockTP[c]) blockTP[c] = s0;
          if (s1 > blockTP[c]) blockTP[c] = s1;
          if (s2 > blockTP[c]) blockTP[c] = s2;
          if (s3 > blockTP[c]) blockTP[c] = s3;

          tmpBufPos++;
          if (tmpBufPos >= tpFiltLen) tmpBufPos = 0;
        }
        if (blockTP[c] > hLoudnessMeter->maxTruePeak[c]) hLoudnessMeter->maxTruePeak[c] = blockTP[c];
      }
      tpBufPos = (tpBufPos + n) % tpFiltLen;
    }
    if (!(hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_SP)) {
      for (c = 0; c < nCh; c++) {
        if (hLoudnessMeter->channelGroup[c] == 0) continue;

        for (i = 0; i < n; i++) {
          float s = (float)fabs(pAudio[i * nCh + c]);
          if (s > blockSP[c]) blockSP[c] = s;
        }
        if (blockSP[c] > hLoudnessMeter->maxSamplePeak[c]) hLoudnessMeter->maxSamplePeak[c] = blockSP[c];
      }
    }

    blockPos += n;
    if (blockPos >= blockLen) {
      float blockNrg = 0.0f, meanNrg;

      for (c = 0; c < nCh; c++) {
        blockNrg += hLoudnessMeter->chNrg[c] * hLoudnessMeter->chWeights[c];
      }

      meanNrg = blockNrg / blockLen;

      hLoudnessMeter->momentBufNrg[hLoudnessMeter->momentBufPos] = meanNrg;
      for (c = 0; c < nCh; c++) {
        hLoudnessMeter->momentBufTP[hLoudnessMeter->momentBufPos][c] = blockTP[c];
        hLoudnessMeter->momentBufSP[hLoudnessMeter->momentBufPos][c] = blockSP[c];
      }
      hLoudnessMeter->shortBufNrg[hLoudnessMeter->shortBufPos] = meanNrg;
      for (c = 0; c < nCh; c++) {
        hLoudnessMeter->shortBufTP[hLoudnessMeter->shortBufPos][c] = blockTP[c];
        hLoudnessMeter->shortBufSP[hLoudnessMeter->shortBufPos][c] = blockSP[c];
      }
      hLoudnessMeter->shortBufPos++;
      if (hLoudnessMeter->shortBufPos >= SHORT_LEN) hLoudnessMeter->shortBufPos = 0;

      hLoudnessMeter->ungatedNrg += meanNrg;

      hLoudnessMeter->momentBufPos++;
      if (hLoudnessMeter->momentBufPos >= MOMENT_LEN) hLoudnessMeter->momentBufPos = 0;

      hLoudnessMeter->ungatedCnt++;

      if (!(hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS)) {
        if (hLoudnessMeter->ungatedCnt >= MOMENT_LEN) {
          float momentNrg;
          int hi;

          momentNrg = hLoudnessMeter->momentBufNrg[0];
          for (i = 1; i < MOMENT_LEN; i++) momentNrg += hLoudnessMeter->momentBufNrg[i];
          momentNrg /= MOMENT_LEN;

          if (momentNrg >= hLoudnessMeter->gateThrAbs) {
            hLoudnessMeter->absgatedNrg += momentNrg;
            hLoudnessMeter->absgatedCnt++;
          }

          hi = NRG2HI(momentNrg);
          hi = min(hi, HIST_SIZE - 1);
          if (hi >= 0) hLoudnessMeter->momentHist[hi]++;

          if (momentNrg > hLoudnessMeter->maxMomentNrg) hLoudnessMeter->maxMomentNrg = momentNrg;

          if (hLoudnessMeter->momentaryEnergy) {
            hLoudnessMeter->momentaryEnergy[hLoudnessMeter->momentaryEnergyIndex] = momentNrg;
            hLoudnessMeter->momentaryEnergyIndex++;
            if (hLoudnessMeter->momentaryEnergyIndex >= hLoudnessMeter->momentaryEnergyLength)
              hLoudnessMeter->momentaryEnergyIndex %= hLoudnessMeter->momentaryEnergyLength;
            hLoudnessMeter->bGatedMidTermLoudnessDirty = 1;
          }
        }

        if (hLoudnessMeter->ungatedCnt >= SHORT_LEN) {
          float shortNrg;
          int hi;

          shortNrg = hLoudnessMeter->shortBufNrg[0];
          for (i = 1; i < SHORT_LEN; i++) shortNrg += hLoudnessMeter->shortBufNrg[i];
          shortNrg /= SHORT_LEN;

          if (shortNrg >= hLoudnessMeter->gateThrAbs) {
            hLoudnessMeter->absgatedNrgLR += shortNrg;
            hLoudnessMeter->absgatedCntLR++;
          }

          hi = NRG2HI(shortNrg);
          hi = min(hi, HIST_SIZE - 1);
          if (hi >= 0) hLoudnessMeter->shortHist[hi]++;

          if (shortNrg > hLoudnessMeter->maxShortNrg) hLoudnessMeter->maxShortNrg = shortNrg;

          if (hLoudnessMeter->hiDelayLine && hLoudnessMeter->midTermShortHist) {
            int old_hi = hLoudnessMeter->hiDelayLine[hLoudnessMeter->hiDelayLineIndex];
            if (old_hi >= 0) {
              assert(hLoudnessMeter->midTermShortHist[old_hi] > 0);
              if (hLoudnessMeter->midTermShortHist[old_hi] > 0)
                hLoudnessMeter->midTermShortHist[old_hi]--;
            }

            hLoudnessMeter->hiDelayLine[hLoudnessMeter->hiDelayLineIndex] = hi;
            hLoudnessMeter->hiDelayLineIndex++;
            if (hLoudnessMeter->hiDelayLineIndex >= hLoudnessMeter->hiDelayLineLength)
              hLoudnessMeter->hiDelayLineIndex %= hLoudnessMeter->hiDelayLineLength;

            if (hi >= 0) hLoudnessMeter->midTermShortHist[hi]++;
            hLoudnessMeter->bMidTermLRADirty = 1;
          }
        }

        for (c = 0; c < nCh; c++) {
          hLoudnessMeter->chNrg[c] = 0.0f;
        }

        for (c = 0; c < nCh; c++) {
          BIQUAD_STATE preS = hLoudnessMeter->preFilt.state[c];
          BIQUAD_STATE rlbS = hLoudnessMeter->rlbFilt.state[c];
          if ((preS.z1 * preS.z1) < EN_LIMIT) preS.z1 = 0.0f;
          if ((preS.z2 * preS.z2) < EN_LIMIT) preS.z2 = 0.0f;
          if ((rlbS.z1 * rlbS.z1) < EN_LIMIT) rlbS.z1 = 0.0f;
          if ((rlbS.z2 * rlbS.z2) < EN_LIMIT) rlbS.z2 = 0.0f;
        }
      }

      blockPos = 0;
      blockNrg = 0;
      for (c = 0; c < nCh; c++) {
        blockTP[c] = 0;
        blockSP[c] = 0;
      }
    }

    pAudio += n * nCh;
    nSamplesLeft -= n;
  }

  hLoudnessMeter->blockPos = blockPos;
  hLoudnessMeter->tpBufPos = tpBufPos;

  return ERRLOUD_Success;
}

float LoudMeter_GetMomentaryLoudness(HLOUDNESS_METER hLoudnessMeter) {
  return NRG2LUFS(LoudMeter_GetMomentaryKWeightedEnergy(hLoudnessMeter));
}

float LoudMeter_GetMomentaryKWeightedEnergy(HLOUDNESS_METER hLoudnessMeter) {
  float momentNrg;
  unsigned int i;

  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return UNDEFINED_LOUDNESS_VALUE;

  momentNrg = hLoudnessMeter->momentBufNrg[0];
  for (i = 1; i < MOMENT_LEN; i++) momentNrg += hLoudnessMeter->momentBufNrg[i];
  momentNrg /= MOMENT_LEN;

  return momentNrg;
}

float LoudMeter_GetShortTermLoudness(HLOUDNESS_METER hLoudnessMeter) {
  return NRG2LUFS(LoudMeter_GetShortTermKWeightedEnergy(hLoudnessMeter));
}

float LoudMeter_GetShortTermKWeightedEnergy(HLOUDNESS_METER hLoudnessMeter) {
  float shortNrg;
  unsigned int i;

  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return UNDEFINED_LOUDNESS_VALUE;

  shortNrg = hLoudnessMeter->shortBufNrg[0];
  for (i = 1; i < SHORT_LEN; i++) shortNrg += hLoudnessMeter->shortBufNrg[i];
  shortNrg /= SHORT_LEN;

  return shortNrg;
}

float LoudMeter_GetUngatedLongTermLoudness(HLOUDNESS_METER hLoudnessMeter) {
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return UNDEFINED_LOUDNESS_VALUE;

  if (!hLoudnessMeter->ungatedCnt) return LOUD_MIN;

  return NRG2LUFS(hLoudnessMeter->ungatedNrg / hLoudnessMeter->ungatedCnt);
}

float LoudMeter_GetGatedLongTermLoudness(HLOUDNESS_METER hLoudnessMeter) {
  unsigned int i;
  unsigned long long n;
  int hi;
  float nrg;

  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return UNDEFINED_LOUDNESS_VALUE;

  if (!hLoudnessMeter->absgatedCnt) return LOUD_MIN;
  hi = NRG2HI(hLoudnessMeter->absgatedNrg / hLoudnessMeter->absgatedCnt);
  hi += GATE_THR_REL * HIST_RES;
  if (hi < 0) hi = 0;

  n = 0;
  nrg = 0;
  for (i = hi; i < HIST_SIZE; i++) {
    n += hLoudnessMeter->momentHist[i];
    nrg += hLoudnessMeter->momentHist[i] * HI2NRG(i);
  }
  if (!n) return LOUD_MIN;
  nrg /= n;

  return NRG2LUFS(nrg);
}

float LoudMeter_GetLongTermGatingThreshold(HLOUDNESS_METER hLoudnessMeter) {
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return UNDEFINED_LOUDNESS_VALUE;
  if (!hLoudnessMeter->absgatedCnt) return LOUD_MIN;
  return max(-70, NRG2LUFS(hLoudnessMeter->absgatedNrg / hLoudnessMeter->absgatedCnt) + GATE_THR_REL);
}

float LoudMeter_GetInstantaneousLoudness(HLOUDNESS_METER hLoudnessMeter) {
  int momentBufPos;
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  momentBufPos = hLoudnessMeter->momentBufPos - 1;
  if (momentBufPos == -1)
    momentBufPos += MOMENT_LEN;
  return NRG2LUFS(hLoudnessMeter->momentBufNrg[momentBufPos]);
}

float LoudMeter_GetInstantaneousKWeightedEnergy(HLOUDNESS_METER hLoudnessMeter) {
  int momentBufPos;
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  momentBufPos = hLoudnessMeter->momentBufPos - 1;
  if (momentBufPos == -1)
    momentBufPos += MOMENT_LEN;
  return hLoudnessMeter->momentBufNrg[momentBufPos];
}

float LoudMeter_GetLoudnessRange(HLOUDNESS_METER hLoudnessMeter) {
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  return LoudMeter_GetLoudnessRange2(hLoudnessMeter, NULL, NULL);
}

float LoudMeter_GetLoudnessRange2(HLOUDNESS_METER hLoudnessMeter, float *pLow, float *pHigh) {
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) {
    if (pLow) *pLow = UNDEFINED_LOUDNESS_VALUE;
    if (pHigh) *pHigh = UNDEFINED_LOUDNESS_VALUE;
    return UNDEFINED_LOUDNESS_VALUE;
  }

  return s_getLRAFromHistogram(hLoudnessMeter->shortHist, hLoudnessMeter->absgatedNrgLR, hLoudnessMeter->absgatedCntLR, pLow, pHigh);
}

float LoudMeter_GetLRAGatingThreshold(HLOUDNESS_METER hLoudnessMeter) {
  if (!hLoudnessMeter->absgatedCntLR) return LOUD_MIN;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return UNDEFINED_LOUDNESS_VALUE;
  return max(-70, NRG2LUFS(hLoudnessMeter->absgatedNrgLR / hLoudnessMeter->absgatedCntLR) + GATE_THR_REL_LR);
}

float LoudMeter_GetMaxTruePeak(HLOUDNESS_METER hLoudnessMeter, int channel) {
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_TP) return UNDEFINED_LOUDNESS_VALUE;

  if ((channel > 0) && (channel <= (int)hLoudnessMeter->nCh)) {
    return LIN2DB(hLoudnessMeter->maxTruePeak[channel - 1]);
  } else {
    unsigned int c;
    float tp = 0;

    for (c = 0; c < hLoudnessMeter->nCh; c++) {
      if (hLoudnessMeter->maxTruePeak[c] > tp) tp = hLoudnessMeter->maxTruePeak[c];
    }

    return LIN2DB(tp);
  }
}

float LoudMeter_GetMomentaryTruePeak(HLOUDNESS_METER hLoudnessMeter, int channel) {
  float tp;
  unsigned int i, c;

  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_TP) return UNDEFINED_LOUDNESS_VALUE;

  if ((channel > 0) && (channel <= (int)hLoudnessMeter->nCh)) {
    tp = hLoudnessMeter->momentBufTP[0][channel - 1];
    for (i = 1; i < MOMENT_LEN; i++) {
      if (hLoudnessMeter->momentBufTP[i][channel - 1] > tp) tp = hLoudnessMeter->momentBufTP[i][channel - 1];
    }
  } else {
    tp = 0;
    for (c = 0; c < hLoudnessMeter->nCh; c++) {
      for (i = 0; i < MOMENT_LEN; i++) {
        if (hLoudnessMeter->momentBufTP[i][c] > tp) tp = hLoudnessMeter->momentBufTP[i][c];
      }
    }
  }

  return LIN2DB(tp);
}

float LoudMeter_GetShortTermTruePeak(HLOUDNESS_METER hLoudnessMeter, int channel) {
  float tp;
  unsigned int i, c;

  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_TP) return UNDEFINED_LOUDNESS_VALUE;

  if ((channel > 0) && (channel <= (int)hLoudnessMeter->nCh)) {
    tp = hLoudnessMeter->shortBufTP[0][channel - 1];
    for (i = 1; i < SHORT_LEN; i++) {
      if (hLoudnessMeter->shortBufTP[i][channel - 1] > tp) tp = hLoudnessMeter->shortBufTP[i][channel - 1];
    }
  } else {
    tp = 0;
    for (c = 0; c < hLoudnessMeter->nCh; c++) {
      for (i = 0; i < SHORT_LEN; i++) {
        if (hLoudnessMeter->shortBufTP[i][c] > tp) tp = hLoudnessMeter->shortBufTP[i][c];
      }
    }
  }

  return LIN2DB(tp);
}

float LoudMeter_GetMaxSamplePeak(HLOUDNESS_METER hLoudnessMeter, int channel) {
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_SP) return UNDEFINED_LOUDNESS_VALUE;

  if ((channel > 0) && (channel <= (int)hLoudnessMeter->nCh)) {
    return LIN2DB(hLoudnessMeter->maxSamplePeak[channel - 1]);
  } else {
    unsigned int c;
    float sp = 0;

    for (c = 0; c < hLoudnessMeter->nCh; c++) {
      if (hLoudnessMeter->maxSamplePeak[c] > sp) sp = hLoudnessMeter->maxSamplePeak[c];
    }

    return LIN2DB(sp);
  }
}

float LoudMeter_GetMomentarySamplePeak(HLOUDNESS_METER hLoudnessMeter, int channel) {
  float sp;
  unsigned int i, c;

  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_SP) return UNDEFINED_LOUDNESS_VALUE;

  if ((channel > 0) && (channel <= (int)hLoudnessMeter->nCh)) {
    sp = hLoudnessMeter->momentBufSP[0][channel - 1];
    for (i = 1; i < MOMENT_LEN; i++) {
      if (hLoudnessMeter->momentBufSP[i][channel - 1] > sp) sp = hLoudnessMeter->momentBufSP[i][channel - 1];
    }
  } else {
    sp = 0;
    for (c = 0; c < hLoudnessMeter->nCh; c++) {
      for (i = 0; i < MOMENT_LEN; i++) {
        if (hLoudnessMeter->momentBufSP[i][c] > sp) sp = hLoudnessMeter->momentBufSP[i][c];
      }
    }
  }

  return LIN2DB(sp);
}

float LoudMeter_GetShortTermSamplePeak(HLOUDNESS_METER hLoudnessMeter, int channel) {
  float sp;
  unsigned int i, c;

  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_SP) return UNDEFINED_LOUDNESS_VALUE;

  if ((channel > 0) && (channel <= (int)hLoudnessMeter->nCh)) {
    sp = hLoudnessMeter->shortBufSP[0][channel - 1];
    for (i = 1; i < SHORT_LEN; i++) {
      if (hLoudnessMeter->shortBufSP[i][channel - 1] > sp) sp = hLoudnessMeter->shortBufSP[i][channel - 1];
    }
  } else {
    sp = 0;
    for (c = 0; c < hLoudnessMeter->nCh; c++) {
      for (i = 0; i < SHORT_LEN; i++) {
        if (hLoudnessMeter->shortBufSP[i][c] > sp) sp = hLoudnessMeter->shortBufSP[i][c];
      }
    }
  }

  return LIN2DB(sp);
}

float LoudMeter_GetMaxMomentaryLoudness(HLOUDNESS_METER hLoudnessMeter) {
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return UNDEFINED_LOUDNESS_VALUE;
  return NRG2LUFS(hLoudnessMeter->maxMomentNrg);
}

float LoudMeter_GetMaxShortTermLoudness(HLOUDNESS_METER hLoudnessMeter) {
  if (!hLoudnessMeter) return UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return UNDEFINED_LOUDNESS_VALUE;
  return NRG2LUFS(hLoudnessMeter->maxShortNrg);
}

unsigned long long LoudMeter_GetMomentaryHistogram(HLOUDNESS_METER hLoudnessMeter, unsigned long long *hist, int minLUFS, int maxLUFS) {
  unsigned int i;
  unsigned long long n = 0;

  if (!hLoudnessMeter || !hist) return (unsigned long long)UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return (unsigned long long)UNDEFINED_LOUDNESS_VALUE;

  if (minLUFS > maxLUFS) return 0;

  for (i = 0; i <= (unsigned int)(maxLUFS - minLUFS); i++) hist[i] = 0;

  for (i = 0; i < HIST_SIZE; i++) {
    int dB = (int)floor((i + 0.5f) / HIST_RES + 0.5f) + GATE_THR_ABS;
    if (dB < minLUFS) continue;
    if (dB > maxLUFS) break;
    hist[dB - minLUFS] += hLoudnessMeter->momentHist[i];
    n += hLoudnessMeter->momentHist[i];
  }

  return n;
}

unsigned long long LoudMeter_GetShortTermHistogram(HLOUDNESS_METER hLoudnessMeter, unsigned long long *hist, int minLUFS, int maxLUFS) {
  unsigned int i;
  unsigned long long n = 0;

  if (!hLoudnessMeter) return (unsigned long long)UNDEFINED_LOUDNESS_VALUE;
  if (hLoudnessMeter->statusWorkload & LOUDMTR_WORKLOAD_DISABLE_LOUDNESS) return (unsigned long long)UNDEFINED_LOUDNESS_VALUE;

  if (minLUFS > maxLUFS) return 0;

  for (i = 0; i <= (unsigned int)(maxLUFS - minLUFS); i++) hist[i] = 0;

  for (i = 0; i < HIST_SIZE; i++) {
    int dB = (int)floor((i + 0.5f) / HIST_RES + 0.5f) + GATE_THR_ABS;
    if (dB < minLUFS) continue;
    if (dB > maxLUFS) break;
    hist[dB - minLUFS] += hLoudnessMeter->shortHist[i];
    n += hLoudnessMeter->shortHist[i];
  }

  return n;
}

const LOUDMTR_LIBINFO *LoudMeter_GetLibraryInfo(void) {
  return &libInfo;
}

static void setKweightFilt(BIQUAD_COEFF *preFilt, BIQUAD_COEFF *rlbFilt, unsigned int fs) {
  float w0, A, alpha, sinw0, cosw0, sqrtA;
  float b0, b1, b2, a0, a1, a2;

  w0 = 2 * PI * 1500 / fs;
  sinw0 = (float)sin(w0);
  cosw0 = (float)cos(w0);
  A = 1.25892541f;
  sqrtA = (float)sqrt(A);
  alpha = (float)(sinw0 * 0.5 * sqrt(2));

  b0 = A * ((A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha);
  b1 = -2 * A * ((A - 1) + (A + 1) * cosw0);
  b2 = A * ((A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha);
  a0 = (A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha;
  a1 = 2 * ((A - 1) - (A + 1) * cosw0);
  a2 = (A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha;

  preFilt->b0 = b0 / a0;
  preFilt->b1 = b1 / a0;
  preFilt->b2 = b2 / a0;
  preFilt->a1 = a1 / a0;
  preFilt->a2 = a2 / a0;

  w0 = 2 * PI * 38 / fs;
  sinw0 = (float)sin(w0);
  cosw0 = (float)cos(w0);
  alpha = sinw0;

  b0 = (1 + cosw0) / 2;
  b1 = -(1 + cosw0);
  b2 = (1 + cosw0) / 2;
  a0 = 1 + alpha;
  a1 = -2 * cosw0;
  a2 = 1 - alpha;

  rlbFilt->b0 = b0 / a0;
  rlbFilt->b1 = b1 / a0;
  rlbFilt->b2 = b2 / a0;
  rlbFilt->a1 = a1 / a0;
  rlbFilt->a2 = a2 / a0;
}

LOUDMTR_ERROR LoudMeter_SetWorkloadConfig(HLOUDNESS_METER hLoudnessMeter,
                                          LOUDMTR_WORKLOAD workloadConfig) {
  LOUDMTR_ERROR err = ERRLOUD_Success;

  if (hLoudnessMeter != NULL) {
    hLoudnessMeter->statusWorkload = workloadConfig;
  } else {
    err = ERRLOUD_NullPointer;
  }

  return err;
}

static int NRG2HI(float nrg) {
  double f = floor((NRG2LUFS(nrg) - GATE_THR_ABS) * HIST_RES);
  if (f < INT_MIN) {
    return INT_MIN;
  } else if (f > INT_MAX) {
    return INT_MAX;
  } else {
    return (int)f;
  }
}

LOUDMTR_ERROR LoudMeter_InitGatedMidTermLoudness(HLOUDNESS_METER hLoudnessMeter, unsigned int length) {
  if (!hLoudnessMeter) {
    return ERRLOUD_NullPointer;
  }
  if (length == 0) {
    if (hLoudnessMeter->momentaryEnergy) free(hLoudnessMeter->momentaryEnergy);
    hLoudnessMeter->momentaryEnergy = NULL;
    hLoudnessMeter->momentaryEnergyLength = 0;
    hLoudnessMeter->momentaryEnergyIndex = 0;
  } else {
    if (hLoudnessMeter->momentaryEnergy) free(hLoudnessMeter->momentaryEnergy);
    hLoudnessMeter->momentaryEnergy = calloc(length, sizeof(float));
    if (!hLoudnessMeter->momentaryEnergy) {
      return ERRLOUD_OutOfMemory;
    }
    hLoudnessMeter->momentaryEnergyLength = length;
    hLoudnessMeter->momentaryEnergyIndex = 0;
    hLoudnessMeter->bGatedMidTermLoudnessDirty = 1;
  }
  return ERRLOUD_Success;
}

float LoudMeter_GetGatedMidTermLoudness(HLOUDNESS_METER hLoudnessMeter) {
  if (hLoudnessMeter->bGatedMidTermLoudnessDirty && hLoudnessMeter->momentaryEnergy) {
    s_CalculateLoudness(hLoudnessMeter->momentaryEnergy, hLoudnessMeter->momentaryEnergyLength, &hLoudnessMeter->gatedMidTermLoudness, &hLoudnessMeter->gatedMidTermLoudnessCount);
    hLoudnessMeter->bGatedMidTermLoudnessDirty = 0;
  }

  if (hLoudnessMeter->momentaryEnergy) {
    return hLoudnessMeter->gatedMidTermLoudness;
  } else {
    return UNDEFINED_LOUDNESS_VALUE;
  }
}

unsigned int LoudMeter_GetGatedMidTermLoudnessCount(HLOUDNESS_METER hLoudnessMeter) {
  if (hLoudnessMeter->bGatedMidTermLoudnessDirty && hLoudnessMeter->momentaryEnergy) {
    LoudMeter_GetGatedMidTermLoudness(hLoudnessMeter);
  }
  if (hLoudnessMeter->momentaryEnergy) {
    return hLoudnessMeter->gatedMidTermLoudnessCount;
  } else {
    return 0;
  }
}

LOUDMTR_ERROR LoudMeter_InitMidTermLoudnessRange(HLOUDNESS_METER hLoudnessMeter, unsigned int length) {
  if (!hLoudnessMeter) {
    return ERRLOUD_NullPointer;
  }
  if (length == 0) {
    if (hLoudnessMeter->hiDelayLine) free(hLoudnessMeter->hiDelayLine);
    hLoudnessMeter->hiDelayLine = NULL;
    if (hLoudnessMeter->midTermShortHist) free(hLoudnessMeter->midTermShortHist);
    hLoudnessMeter->midTermShortHist = NULL;
    hLoudnessMeter->hiDelayLineLength = 0;
    hLoudnessMeter->hiDelayLineIndex = 0;
  } else {
    unsigned int i;
    if (hLoudnessMeter->hiDelayLine) free(hLoudnessMeter->hiDelayLine);
    hLoudnessMeter->hiDelayLine = calloc(length, sizeof(float));
    if (!hLoudnessMeter->hiDelayLine) {
      return ERRLOUD_OutOfMemory;
    }
    for (i = 0; i < length; i++) {
      hLoudnessMeter->hiDelayLine[i] = -1;
    }
    if (hLoudnessMeter->midTermShortHist) free(hLoudnessMeter->midTermShortHist);
    hLoudnessMeter->midTermShortHist = calloc(HIST_SIZE, sizeof(unsigned long long));
    if (!hLoudnessMeter->midTermShortHist) {
      return ERRLOUD_OutOfMemory;
    }
    hLoudnessMeter->hiDelayLineLength = length;
    hLoudnessMeter->hiDelayLineIndex = 0;
    hLoudnessMeter->bMidTermLRADirty = 1;
  }
  return ERRLOUD_Success;
}

float LoudMeter_GetMidTermLoudnessRange(HLOUDNESS_METER hLoudnessMeter) {
  return LoudMeter_GetMidTermLoudnessRange2(hLoudnessMeter, NULL, NULL);
}

float LoudMeter_GetMidTermLoudnessRange2(HLOUDNESS_METER hLoudnessMeter,
                                         float *pLow,
                                         float *pHigh) {
  if (hLoudnessMeter->bMidTermLRADirty && hLoudnessMeter->hiDelayLine && hLoudnessMeter->midTermShortHist) {
    hLoudnessMeter->midTermLRA = s_CalculateLRA(hLoudnessMeter->midTermShortHist, hLoudnessMeter->gateThrAbs, &hLoudnessMeter->midTermLRACount, &hLoudnessMeter->midTermLRALow, &hLoudnessMeter->midTermLRAHigh);
    hLoudnessMeter->bMidTermLRADirty = 0;
  }

  if (hLoudnessMeter->hiDelayLine && hLoudnessMeter->midTermShortHist) {
    if (pLow) *pLow = hLoudnessMeter->midTermLRALow;
    if (pHigh) *pHigh = hLoudnessMeter->midTermLRAHigh;
    return hLoudnessMeter->midTermLRA;
  } else {
    return UNDEFINED_LOUDNESS_VALUE;
  }
}

unsigned int LoudMeter_GetMidTermLoudnessRangeCount(HLOUDNESS_METER hLoudnessMeter) {
  if (hLoudnessMeter->bMidTermLRADirty && hLoudnessMeter->hiDelayLine && hLoudnessMeter->midTermShortHist) {
    LoudMeter_GetMidTermLoudnessRange(hLoudnessMeter);
  }
  if (hLoudnessMeter->hiDelayLine && hLoudnessMeter->midTermShortHist) {
    return hLoudnessMeter->midTermLRACount;
  } else {
    return 0;
  }
}

static LOUDMTR_ERROR s_CalculateLoudness(const float *momentaryEnergy,
                                         const unsigned int momentaryEnergyLength,
                                         float *gatedLoudnessOut,
                                         unsigned int *gatedCountOut) {
  unsigned int i = 0;
  unsigned int counter = 0;

  float absGatedEnergy = 0.0f;
  float gatedEnergy = 0.0f;

  float relEnergyThreshold = 0.0f;
  float gatedLoudness = 0.0f;

  float gate_thr_abs_nrg = 0.0f;

  if (!momentaryEnergy) {
    return ERRLOUD_NullPointer;
  }

  gate_thr_abs_nrg = LUFS2NRG(GATE_THR_ABS);

  for (i = 0; i < momentaryEnergyLength; i++) {
    if (momentaryEnergy[i] >= gate_thr_abs_nrg) {
      absGatedEnergy += momentaryEnergy[i];
      counter++;
    }
  }

  if (counter == 0) {
    gatedLoudness = LOUD_MIN;
  } else {
    absGatedEnergy /= (float)counter;
    relEnergyThreshold = LUFS2NRG(NRG2LUFS(absGatedEnergy) + GATE_THR_REL);
    if (relEnergyThreshold > gate_thr_abs_nrg) {
      counter = 0;
      for (i = 0; i < momentaryEnergyLength; i++) {
        if (momentaryEnergy[i] >= relEnergyThreshold) {
          gatedEnergy += momentaryEnergy[i];
          counter++;
        }
      }
      if (counter) {
        gatedEnergy /= (float)counter;
        gatedLoudness = NRG2LUFS(gatedEnergy);
      } else {
        gatedLoudness = LOUD_MIN;
      }
    } else {
      gatedLoudness = NRG2LUFS(absGatedEnergy);
    }
  }

  if (gatedLoudnessOut) *gatedLoudnessOut = gatedLoudness;
  if (gatedCountOut) *gatedCountOut = counter;

  return ERRLOUD_Success;
}

static float s_getLRAFromHistogram(unsigned long long *shortHist,
                                   float absGatedNrgLR,
                                   unsigned long long absGatedCntLR,
                                   float *pLow,
                                   float *pHigh) {
  unsigned int i, pl = 0, ph = 0;
  unsigned long long n, p;
  int hi;

  if (!shortHist) return UNDEFINED_LOUDNESS_VALUE;

  if (!absGatedCntLR) {
    if (pLow) *pLow = GATE_THR_ABS;
    if (pHigh) *pHigh = GATE_THR_ABS;
    return 0;
  }

  hi = NRG2HI(absGatedNrgLR / absGatedCntLR);
  hi += GATE_THR_REL_LR * HIST_RES;
  if (hi < 0) hi = 0;

  n = 0;
  for (i = hi; i < HIST_SIZE; i++) {
    n += shortHist[i];
  }

  p = 0;
  for (i = hi; i < HIST_SIZE; i++) {
    p += shortHist[i];
    if (p * 100 >= n * PERC_LO) break;
  }
  pl = i;

  if (p * 100 >= n * PERC_HI) {
    ph = pl;
  } else {
    for (i = i + 1; i < HIST_SIZE; i++) {
      p += shortHist[i];
      if (p * 100 >= n * PERC_HI) break;
    }
    ph = i;
  }

  if (pLow) *pLow = (float)pl / HIST_RES + GATE_THR_ABS;
  if (pHigh) *pHigh = (float)ph / HIST_RES + GATE_THR_ABS;

  return (float)(ph - pl) / HIST_RES;
}

static float s_CalculateLRA(unsigned long long *midTermShortHist, const float gateThrAbs, unsigned int *lraCount, float *pLow, float *pHigh) {
  int i, hi;
  float nrg = 0.0f;
  unsigned long long n = 0;

  hi = NRG2HI(gateThrAbs);
  for (i = hi; i < HIST_SIZE; i++) {
    n += midTermShortHist[i];
    nrg += midTermShortHist[i] * HI2NRG(i);
  }

  if (lraCount) *lraCount = (unsigned int)n;

  return s_getLRAFromHistogram(midTermShortHist, nrg, n, pLow, pHigh);
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
