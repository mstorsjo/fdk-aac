
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
#include <assert.h>
#include "cfft.h"
#include "iisfft.h"
#include "iisutillib.h"
#include "cpuinfo.h"

#define MAX_FFT_SIZE 1024

#define MAX_TRIGDATA_SIZE (MAX_FFT_SIZE / 2)
#define MODULO_16(X) (((size_t)(X)) & 15u)
#define HEXBYTE_ALIGN_CHECK(a) (0 == MODULO_16((size_t)(a)))

#if defined _MSC_VER && (_MSC_VER == 1900) && !defined LINEAR_COEFF_TABLE
#error C99 hexadecimal floating point constants not supported in VS 2015
#endif

ALIGN_16_BYTE
static const float static_table[MAX_TRIGDATA_SIZE + 2] = {
    0x1p+0,
    0x0p+0,
    0x1.ffff62p-1,
    0x1.921f8cp-9,
    0x1.fffd88p-1,
    0x1.921f1p-8,
    0x1.fffa72p-1,
    0x1.2d96bp-7,
    0x1.fff622p-1,
    0x1.921d2p-7,
    0x1.fff094p-1,
    0x1.f6a296p-7,
    0x1.ffe9ccp-1,
    0x1.2d936cp-6,
    0x1.ffe1c6p-1,
    0x1.5fd4d2p-6,
    0x1.ffd886p-1,
    0x1.92156p-6,
    0x1.ffce0ap-1,
    0x1.c454f4p-6,
    0x1.ffc252p-1,
    0x1.f69374p-6,
    0x1.ffb55ep-1,
    0x1.14685ep-5,
    0x1.ffa72ep-1,
    0x1.2d8658p-5,
    0x1.ff97c4p-1,
    0x1.46a396p-5,
    0x1.ff871ep-1,
    0x1.5fc00ep-5,
    0x1.ff753cp-1,
    0x1.78dbaap-5,
    0x1.ff621ep-1,
    0x1.91f66p-5,
    0x1.ff4dc6p-1,
    0x1.ab101cp-5,
    0x1.ff383p-1,
    0x1.c428d2p-5,
    0x1.ff2162p-1,
    0x1.dd407p-5,
    0x1.ff0956p-1,
    0x1.f656e8p-5,
    0x1.fef01p-1,
    0x1.07b614p-4,
    0x1.fed58ep-1,
    0x1.144014p-4,
    0x1.feb9d2p-1,
    0x1.20c968p-4,
    0x1.fe9cdap-1,
    0x1.2d520ap-4,
    0x1.fe7ea8p-1,
    0x1.39d9f2p-4,
    0x1.fe5f3ap-1,
    0x1.466118p-4,
    0x1.fe3e92p-1,
    0x1.52e774p-4,
    0x1.fe1cbp-1,
    0x1.5f6dp-4,
    0x1.fdf992p-1,
    0x1.6bf1b4p-4,
    0x1.fdd53ap-1,
    0x1.787586p-4,
    0x1.fdafa8p-1,
    0x1.84f872p-4,
    0x1.fd88dap-1,
    0x1.917a6cp-4,
    0x1.fd60d2p-1,
    0x1.9dfb6ep-4,
    0x1.fd3792p-1,
    0x1.aa7b72p-4,
    0x1.fd0d16p-1,
    0x1.b6fa6ep-4,
    0x1.fce16p-1,
    0x1.c3785cp-4,
    0x1.fcb47p-1,
    0x1.cff534p-4,
    0x1.fc8646p-1,
    0x1.dc70ecp-4,
    0x1.fc56e4p-1,
    0x1.e8eb8p-4,
    0x1.fc2648p-1,
    0x1.f564e6p-4,
    0x1.fbf47p-1,
    0x1.00ee8ap-3,
    0x1.fbc162p-1,
    0x1.072a04p-3,
    0x1.fb8d18p-1,
    0x1.0d64dcp-3,
    0x1.fb5798p-1,
    0x1.139f0cp-3,
    0x1.fb20dcp-1,
    0x1.19d894p-3,
    0x1.fae8e8p-1,
    0x1.20116ep-3,
    0x1.faafbcp-1,
    0x1.264994p-3,
    0x1.fa7558p-1,
    0x1.2c8106p-3,
    0x1.fa39bap-1,
    0x1.32b7cp-3,
    0x1.f9fce6p-1,
    0x1.38edbcp-3,
    0x1.f9bed8p-1,
    0x1.3f22f6p-3,
    0x1.f97f92p-1,
    0x1.45576cp-3,
    0x1.f93f14p-1,
    0x1.4b8b18p-3,
    0x1.f8fd6p-1,
    0x1.51bdf8p-3,
    0x1.f8ba74p-1,
    0x1.57f008p-3,
    0x1.f8765p-1,
    0x1.5e2144p-3,
    0x1.f830f4p-1,
    0x1.6451a8p-3,
    0x1.f7ea62p-1,
    0x1.6a813p-3,
    0x1.f7a29ap-1,
    0x1.70afd8p-3,
    0x1.f7599ap-1,
    0x1.76dd9ep-3,
    0x1.f70f64p-1,
    0x1.7d0a7cp-3,
    0x1.f6c3f8p-1,
    0x1.83366ep-3,
    0x1.f67756p-1,
    0x1.896172p-3,
    0x1.f6297cp-1,
    0x1.8f8b84p-3,
    0x1.f5da6ep-1,
    0x1.95b49ep-3,
    0x1.f58a2cp-1,
    0x1.9bdccp-3,
    0x1.f538b2p-1,
    0x1.a203e2p-3,
    0x1.f4e604p-1,
    0x1.a82a02p-3,
    0x1.f4922p-1,
    0x1.ae4f1ep-3,
    0x1.f43d08p-1,
    0x1.b4732ep-3,
    0x1.f3e6bcp-1,
    0x1.ba9634p-3,
    0x1.f38f3ap-1,
    0x1.c0b826p-3,
    0x1.f33686p-1,
    0x1.c6d906p-3,
    0x1.f2dc9cp-1,
    0x1.ccf8ccp-3,
    0x1.f2818p-1,
    0x1.d31774p-3,
    0x1.f2253p-1,
    0x1.d934fep-3,
    0x1.f1c7acp-1,
    0x1.df5164p-3,
    0x1.f168f6p-1,
    0x1.e56ca2p-3,
    0x1.f1090cp-1,
    0x1.eb86b4p-3,
    0x1.f0a7fp-1,
    0x1.f19f98p-3,
    0x1.f045a2p-1,
    0x1.f7b748p-3,
    0x1.efe22p-1,
    0x1.fdcdc2p-3,
    0x1.ef7d6ep-1,
    0x1.01f18p-2,
    0x1.ef178ap-1,
    0x1.04fb8p-2,
    0x1.eeb074p-1,
    0x1.0804ep-2,
    0x1.ee482ep-1,
    0x1.0b0d9cp-2,
    0x1.eddeb6p-1,
    0x1.0e15b4p-2,
    0x1.ed740ep-1,
    0x1.111d26p-2,
    0x1.ed0836p-1,
    0x1.1423eep-2,
    0x1.ec9b2ep-1,
    0x1.172a0ep-2,
    0x1.ec2cf4p-1,
    0x1.1a2f8p-2,
    0x1.ebbd8cp-1,
    0x1.1d3444p-2,
    0x1.eb4cf6p-1,
    0x1.203858p-2,
    0x1.eadb2ep-1,
    0x1.233bbap-2,
    0x1.ea683ap-1,
    0x1.263e6ap-2,
    0x1.e9f416p-1,
    0x1.294062p-2,
    0x1.e97ec4p-1,
    0x1.2c41a4p-2,
    0x1.e90844p-1,
    0x1.2f422ep-2,
    0x1.e89096p-1,
    0x1.3241fcp-2,
    0x1.e817bap-1,
    0x1.35410cp-2,
    0x1.e79db2p-1,
    0x1.383f5ep-2,
    0x1.e7227ep-1,
    0x1.3b3cfp-2,
    0x1.e6a61cp-1,
    0x1.3e39bep-2,
    0x1.e6288ep-1,
    0x1.4135cap-2,
    0x1.e5a9d6p-1,
    0x1.44310ep-2,
    0x1.e529fp-1,
    0x1.472b8ap-2,
    0x1.e4a8ep-1,
    0x1.4a253ep-2,
    0x1.e426a4p-1,
    0x1.4d1e24p-2,
    0x1.e3a33ep-1,
    0x1.50163ep-2,
    0x1.e31eaep-1,
    0x1.530d88p-2,
    0x1.e298f4p-1,
    0x1.560402p-2,
    0x1.e2121p-1,
    0x1.58f9a8p-2,
    0x1.e18a02p-1,
    0x1.5bee78p-2,
    0x1.e100ccp-1,
    0x1.5ee274p-2,
    0x1.e0766ep-1,
    0x1.61d596p-2,
    0x1.dfeae6p-1,
    0x1.64c7dep-2,
    0x1.df5e36p-1,
    0x1.67b94ap-2,
    0x1.ded06p-1,
    0x1.6aa9d8p-2,
    0x1.de416p-1,
    0x1.6d9986p-2,
    0x1.ddb13cp-1,
    0x1.708854p-2,
    0x1.dd1ffp-1,
    0x1.73763cp-2,
    0x1.dc8d7cp-1,
    0x1.76634p-2,
    0x1.dbf9e4p-1,
    0x1.794f5ep-2,
    0x1.db6526p-1,
    0x1.7c3a94p-2,
    0x1.dacf42p-1,
    0x1.7f24dep-2,
    0x1.da383ap-1,
    0x1.820e3cp-2,
    0x1.d9a00ep-1,
    0x1.84f6aap-2,
    0x1.d906bcp-1,
    0x1.87de2ap-2,
    0x1.d86c48p-1,
    0x1.8ac4b8p-2,
    0x1.d7d0bp-1,
    0x1.8daa52p-2,
    0x1.d733f6p-1,
    0x1.908ef8p-2,
    0x1.d69618p-1,
    0x1.9372a6p-2,
    0x1.d5f718p-1,
    0x1.96555cp-2,
    0x1.d556f6p-1,
    0x1.993716p-2,
    0x1.d4b5b2p-1,
    0x1.9c17d4p-2,
    0x1.d4134ep-1,
    0x1.9ef794p-2,
    0x1.d36fc8p-1,
    0x1.a1d654p-2,
    0x1.d2cb22p-1,
    0x1.a4b412p-2,
    0x1.d2255cp-1,
    0x1.a790cep-2,
    0x1.d17e78p-1,
    0x1.aa6c82p-2,
    0x1.d0d672p-1,
    0x1.ad4732p-2,
    0x1.d02d5p-1,
    0x1.b020d6p-2,
    0x1.cf830ep-1,
    0x1.b2f972p-2,
    0x1.ced7bp-1,
    0x1.b5d1p-2,
    0x1.ce2b32p-1,
    0x1.b8a782p-2,
    0x1.cd7d98p-1,
    0x1.bb7cf2p-2,
    0x1.cccee2p-1,
    0x1.be5152p-2,
    0x1.cc1f1p-1,
    0x1.c1249ep-2,
    0x1.cb6e2p-1,
    0x1.c3f6d4p-2,
    0x1.cabc16p-1,
    0x1.c6c7f4p-2,
    0x1.ca08f2p-1,
    0x1.c997fcp-2,
    0x1.c954b2p-1,
    0x1.cc66eap-2,
    0x1.c89f58p-1,
    0x1.cf34bap-2,
    0x1.c7e8e6p-1,
    0x1.d2016ep-2,
    0x1.c73158p-1,
    0x1.d4cd02p-2,
    0x1.c678b4p-1,
    0x1.d79776p-2,
    0x1.c5bef6p-1,
    0x1.da60c6p-2,
    0x1.c5042p-1,
    0x1.dd28f2p-2,
    0x1.c44834p-1,
    0x1.dfeff6p-2,
    0x1.c38b3p-1,
    0x1.e2b5d4p-2,
    0x1.c2cd14p-1,
    0x1.e57a86p-2,
    0x1.c20de4p-1,
    0x1.e83e0ep-2,
    0x1.c14d9ep-1,
    0x1.eb006ap-2,
    0x1.c08c42p-1,
    0x1.edc196p-2,
    0x1.bfc9d2p-1,
    0x1.f0819p-2,
    0x1.bf064ep-1,
    0x1.f3405ap-2,
    0x1.be41b6p-1,
    0x1.f5fdeep-2,
    0x1.bd7c0ap-1,
    0x1.f8ba4ep-2,
    0x1.bcb54cp-1,
    0x1.fb7576p-2,
    0x1.bbed7cp-1,
    0x1.fe2f64p-2,
    0x1.bb249ap-1,
    0x1.00740cp-1,
    0x1.ba5aa6p-1,
    0x1.01cfc8p-1,
    0x1.b98fa2p-1,
    0x1.032ae6p-1,
    0x1.b8c38ep-1,
    0x1.048562p-1,
    0x1.b7f668p-1,
    0x1.05df3ep-1,
    0x1.b72834p-1,
    0x1.07387ap-1,
    0x1.b658f2p-1,
    0x1.089112p-1,
    0x1.b588ap-1,
    0x1.09e908p-1,
    0x1.b4b74p-1,
    0x1.0b4058p-1,
    0x1.b3e4d4p-1,
    0x1.0c9704p-1,
    0x1.b3115ap-1,
    0x1.0ded0cp-1,
    0x1.b23cd4p-1,
    0x1.0f426cp-1,
    0x1.b16742p-1,
    0x1.109724p-1,
    0x1.b090a6p-1,
    0x1.11eb36p-1,
    0x1.afb8fep-1,
    0x1.133e9cp-1,
    0x1.aee04cp-1,
    0x1.14915ap-1,
    0x1.ae069p-1,
    0x1.15e36ep-1,
    0x1.ad2bcap-1,
    0x1.1734d6p-1,
    0x1.ac4ffcp-1,
    0x1.188592p-1,
    0x1.ab7326p-1,
    0x1.19d5ap-1,
    0x1.aa9548p-1,
    0x1.1b2502p-1,
    0x1.a9b662p-1,
    0x1.1c73b4p-1,
    0x1.a8d676p-1,
    0x1.1dc1b6p-1,
    0x1.a7f586p-1,
    0x1.1f0f08p-1,
    0x1.a7138ep-1,
    0x1.205baap-1,
    0x1.a63092p-1,
    0x1.21a79ap-1,
    0x1.a54c92p-1,
    0x1.22f2d6p-1,
    0x1.a4678cp-1,
    0x1.243d6p-1,
    0x1.a38184p-1,
    0x1.258734p-1,
    0x1.a29a7ap-1,
    0x1.26d054p-1,
    0x1.a1b26ep-1,
    0x1.2818bep-1,
    0x1.a0c95ep-1,
    0x1.296072p-1,
    0x1.9fdf5p-1,
    0x1.2aa76ep-1,
    0x1.9ef43ep-1,
    0x1.2bedb2p-1,
    0x1.9e082ep-1,
    0x1.2d333ep-1,
    0x1.9d1b2p-1,
    0x1.2e780ep-1,
    0x1.9c2d12p-1,
    0x1.2fbc24p-1,
    0x1.9b3e04p-1,
    0x1.30ff8p-1,
    0x1.9a4dfap-1,
    0x1.32421ep-1,
    0x1.995cf2p-1,
    0x1.3384p-1,
    0x1.986afp-1,
    0x1.34c526p-1,
    0x1.9777fp-1,
    0x1.36058cp-1,
    0x1.9683f4p-1,
    0x1.374532p-1,
    0x1.958efep-1,
    0x1.388418p-1,
    0x1.94990ep-1,
    0x1.39c23ep-1,
    0x1.93a224p-1,
    0x1.3affa2p-1,
    0x1.92aa42p-1,
    0x1.3c3c44p-1,
    0x1.91b166p-1,
    0x1.3d7824p-1,
    0x1.90b794p-1,
    0x1.3eb33ep-1,
    0x1.8fbccap-1,
    0x1.3fed96p-1,
    0x1.8ec10ap-1,
    0x1.412726p-1,
    0x1.8dc454p-1,
    0x1.425ff2p-1,
    0x1.8cc6a8p-1,
    0x1.4397f6p-1,
    0x1.8bc806p-1,
    0x1.44cf32p-1,
    0x1.8ac872p-1,
    0x1.4605a6p-1,
    0x1.89c7eap-1,
    0x1.473b52p-1,
    0x1.88c66ep-1,
    0x1.487034p-1,
    0x1.87c4p-1,
    0x1.49a44ap-1,
    0x1.86c0a2p-1,
    0x1.4ad796p-1,
    0x1.85bc52p-1,
    0x1.4c0a14p-1,
    0x1.84b712p-1,
    0x1.4d3bc6p-1,
    0x1.83b0ep-1,
    0x1.4e6cacp-1,
    0x1.82a9c2p-1,
    0x1.4f9cc2p-1,
    0x1.81a1b4p-1,
    0x1.50cc0ap-1,
    0x1.8098b8p-1,
    0x1.51fa82p-1,
    0x1.7f8ecep-1,
    0x1.53282ap-1,
    0x1.7e83f8p-1,
    0x1.5455p-1,
    0x1.7d7836p-1,
    0x1.558104p-1,
    0x1.7c6b8ap-1,
    0x1.56ac36p-1,
    0x1.7b5df2p-1,
    0x1.57d694p-1,
    0x1.7a4f7p-1,
    0x1.59001ep-1,
    0x1.794006p-1,
    0x1.5a28d2p-1,
    0x1.782fb2p-1,
    0x1.5b50b2p-1,
    0x1.771e76p-1,
    0x1.5c77bcp-1,
    0x1.760c52p-1,
    0x1.5d9deep-1,
    0x1.74f948p-1,
    0x1.5ec34ap-1,
    0x1.73e558p-1,
    0x1.5fe7ccp-1,
    0x1.72d084p-1,
    0x1.610b76p-1,
    0x1.71bacap-1,
    0x1.622e44p-1,
    0x1.70a42cp-1,
    0x1.63503ap-1,
    0x1.6f8caap-1,
    0x1.647154p-1,
    0x1.6e7446p-1,
    0x1.659192p-1,
    0x1.6d5afep-1,
    0x1.66b0f4p-1,
    0x1.6c40d8p-1,
    0x1.67cf78p-1,
    0x1.6b25cep-1,
    0x1.68ed1ep-1,
    0x1.6a09e6p-1,
    0x1.6a09e6p-1,
};

static void scramble(float *re, float *im, int n, int s) {
  float tmp;
  int m, k, j;

  for (m = 1, j = 0; m < (n - 1); m++) {
    {
      for (k = n >> 1; (!((j ^= k) & k)); k >>= 1)
        ;
    }

    if (j > m) {
      tmp = re[s * m];
      re[s * m] = re[s * j];
      re[s * j] = tmp;

      tmp = im[s * m];
      im[s * m] = im[s * j];
      im[s * j] = tmp;
    }
  }
}

static inline void
r2fftKernel(float *re1, float *im1, float *re2, float *im2, float c1, float c2) {
  float vr, vi, ur, ui;

  vr = *re2 * c1 + *im2 * c2;
  vi = -*re2 * c2 + *im2 * c1;

  ur = *re1;
  ui = *im1;

  *re1 = ur + vr;
  *im1 = ui + vi;

  *re2 = ur - vr;
  *im2 = ui - vi;
}

static void fft(const float *aTrigData, int trigdata_size, float *re, float *im, int sizeOfFft, int s) {
  int trigstep, i, ldm, n;
  int ldn = 0;

  while (sizeOfFft >>= 1) {
    ldn++;
  }

  n = 1 << ldn;

  scramble(re, im, n, s);

  for (i = 0; i < n; i += 4) {
    float a00, a01, a10, a11;
    float a20, a21, a30, a31;

    a00 = re[s * (i + 0)] + re[s * (i + 1)];
    a10 = re[s * (i + 2)] + re[s * (i + 3)];
    a20 = im[s * (i + 0)] + im[s * (i + 1)];
    a30 = im[s * (i + 2)] + im[s * (i + 3)];

    a01 = re[s * (i + 0)] - re[s * (i + 1)];
    a21 = re[s * (i + 2)] - re[s * (i + 3)];
    a31 = im[s * (i + 0)] - im[s * (i + 1)];
    a11 = im[s * (i + 2)] - im[s * (i + 3)];

    re[s * (i + 0)] = a00 + a10;
    re[s * (i + 2)] = a00 - a10;
    im[s * (i + 0)] = a20 + a30;
    im[s * (i + 2)] = a20 - a30;
    re[s * (i + 1)] = a11 + a01;
    re[s * (i + 3)] = a01 - a11;
    im[s * (i + 1)] = a31 - a21;
    im[s * (i + 3)] = a21 + a31;
  }

  for (ldm = 3; ldm <= ldn; ++ldm) {
    const int m = (1 << ldm);
    const int mh = (m >> 1);
    int j, r;

    trigstep = (trigdata_size * 4) >> ldm;

    {
      float c1, c2;

      c1 = aTrigData[2 * mh / 4 * trigstep];
      c2 = aTrigData[2 * mh / 4 * trigstep + 1];
      for (r = 0; r < n; r += m) {
        int t0 = r;
        int t1 = s * t0;
        int t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, 1.f, 0.f);
        t0 = r + mh / 2;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, -0.f, 1.f);
        t0 = r + mh / 4;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, c1, c2);
        t0 = r + 3 * mh / 4;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, -c2, c1);
      }
    }
    for (j = 1; j < mh / 4; ++j) {
      float c1, c2;
      c1 = aTrigData[2 * j * trigstep];
      c2 = aTrigData[2 * j * trigstep + 1];
      for (r = 0; r < n; r += m) {
        int t0 = r + j;
        int t1 = s * t0;
        int t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, c1, c2);
        t0 = r + j + mh / 2;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, -c2, c1);
        t0 = r + mh / 2 - j;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, c2, c1);
        t0 = r + mh - j;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, -c1, c2);
      }
    }
  }
}

static void ifft(const float *aTrigData, int trigdata_size, float *re, float *im, int sizeOfFft, int s) {
  int trigstep, i, ldm, n;
  int ldn = 0;

  while (sizeOfFft >>= 1) {
    ldn++;
  }

  n = 1 << ldn;

  scramble(re, im, n, s);

  for (i = 0; i < n; i += 4) {
    float a00, a01, a10, a11;
    float a20, a21, a30, a31;

    a00 = re[s * (i + 0)] + re[s * (i + 1)];
    a10 = re[s * (i + 2)] + re[s * (i + 3)];
    a20 = im[s * (i + 0)] + im[s * (i + 1)];
    a30 = im[s * (i + 2)] + im[s * (i + 3)];

    a01 = re[s * (i + 0)] - re[s * (i + 1)];
    a21 = re[s * (i + 2)] - re[s * (i + 3)];
    a31 = im[s * (i + 0)] - im[s * (i + 1)];
    a11 = im[s * (i + 2)] - im[s * (i + 3)];

    re[s * (i + 0)] = a00 + a10;
    re[s * (i + 2)] = a00 - a10;
    im[s * (i + 0)] = a20 + a30;
    im[s * (i + 2)] = a20 - a30;

    re[s * (i + 1)] = a01 - a11;
    re[s * (i + 3)] = a01 + a11;
    im[s * (i + 1)] = a31 + a21;
    im[s * (i + 3)] = a31 - a21;
  }

  for (ldm = 3; ldm <= ldn; ++ldm) {
    const int m = (1 << ldm);
    const int mh = (m >> 1);
    int j, r;

    trigstep = (trigdata_size * 4) >> ldm;

    {
      float c1, c2;
      j = 0;

      c1 = aTrigData[2 * mh / 4 * trigstep];
      c2 = aTrigData[2 * mh / 4 * trigstep + 1];

      for (r = 0; r < n; r += m) {
        int t0 = r;
        int t1 = s * t0;
        int t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, 1.f, 0.f);
        t0 = r + mh / 2;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, 0.f, -1.f);
        t0 = r + mh / 4 + j;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, c1, -c2);
        t0 = r + j + 3 * mh / 4;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, -c2, -c1);
      }
    }
    for (j = 1; j < mh / 4; ++j) {
      float c1, c2;
      c1 = aTrigData[2 * j * trigstep];
      c2 = aTrigData[2 * j * trigstep + 1];

      for (r = 0; r < n; r += m) {
        int t0 = r + j;
        int t1 = s * t0;
        int t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, c1, -c2);
        t0 = r + j + mh / 2;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, -c2, -c1);
        t0 = r + mh / 2 - j;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, c2, -c1);
        t0 = r + mh - j;
        t1 = s * t0;
        t2 = s * (t0 + mh);
        r2fftKernel(re + t1, im + t1, re + t2, im + t2, -c1, -c2);
      }
    }
  }
}

#if defined(FUNCTION_dit_fft)
static int getlog2(int n) {
  int ldn = 0;
  while (n >>= 1) {
    ldn++;
  }

  return ldn;
}
#endif

void cfft(float *re, float *im, int length, int stride, int sign) {
  assert(abs(sign) == 1);
  assert(CFFT_SUPPORT(length));

  if (sign == -1) {
#if defined(FUNCTION_dit_fft)
    if ((stride == 2) && (re + 1 == im) && (HEXBYTE_ALIGN_CHECK(re))) {
      dit_fft(re, getlog2(length), static_table, MAX_TRIGDATA_SIZE);
      return;
    }
#endif
    fft(static_table, MAX_TRIGDATA_SIZE, re, im, length, stride);
  } else {
#if defined(FUNCTION_dit_fft)
    if ((stride == 2) && (re + 1 == im) && (HEXBYTE_ALIGN_CHECK(re))) {
      dit_ifft(re, getlog2(length), static_table, MAX_TRIGDATA_SIZE);
      return;
    }
#endif
    ifft(static_table, MAX_TRIGDATA_SIZE, re, im, length, stride);
  }
}

int cfft_plan(Cfft *handle, int length, int sign) {
  if (!CFFT_PLAN_SUPPORT(length) || abs(sign) != 1)
    return 0;

  handle->len = length;
  handle->sign = sign;

  if (length <= MAX_FFT_SIZE) {
    handle->table = NULL;
    handle->tableSize = 0;
  } else {
    int i = 0;
    handle->table = (float *)iisMalloc((length / 2 + 2) * sizeof(float));
    handle->tableSize = length / 2;
    for (i = 0; i <= length / 4; ++i) {
      handle->table[2 * i] = (float)cos(M_PI * i / (length));
      handle->table[2 * i + 1] = (float)sin(M_PI * i / (length));
    }
  }
  return 1;
}

void cfft_apply(Cfft *handle, float *re, float *im, int stride) {
  if (handle->len <= MAX_FFT_SIZE) {
    cfft(re, im, handle->len, stride, handle->sign);
  } else if (handle->sign == -1) {
#if defined(FUNCTION_dit_fft)
    if ((stride == 2) && (re + 1 == im) && (HEXBYTE_ALIGN_CHECK(re))) {
      dit_fft(re, getlog2(handle->len), handle->table, handle->tableSize);
      return;
    } else
#endif
    {
      fft(handle->table, handle->tableSize, re, im, handle->len, stride);
    }
  } else {
#if defined(FUNCTION_dit_fft)
    if ((stride == 2) && (re + 1 == im) && (HEXBYTE_ALIGN_CHECK(re))) {
      dit_ifft(re, getlog2(handle->len), handle->table, handle->tableSize);
      return;
    } else
#endif
    {
      ifft(handle->table, handle->tableSize, re, im, handle->len, stride);
    }
  }
}

void cfft_free(Cfft *handle) {
  if (handle->table) {
    iisFree(handle->table);
  }
}
