
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

#ifndef IISLPDCOMLIB_CONSTANTS_H
#define IISLPDCOMLIB_CONSTANTS_H

#include "options.h"

#define L_FRAME_1024 1024
#define L_DIV_1024 256
#define NB_DIV 4
#define NB_SUBFR_1024 4
#define L_SUBFR 64
#define NB_SUBFR_SUPERFR_1024 (NB_DIV * NB_SUBFR_1024)

#define LFAC_1024 (L_DIV_1024 / 2)

#define FDNS_NPTS_1024 64

#define L_NEXT_HIGH_RATE_1024 288
#define L_LPC0_1024 256
#define L_WINDOW_1024 448
#define L_WINDOW_HIGH_RATE_1024 512

#define FSCALE_DENOM 12800
#define FAC_FSCALE_MAX 24000
#define FAC_FSCALE_MIN 6000

#define L_TOTAL_HIGH_RATE (M + L_FRAME_1024 + L_NEXT_HIGH_RATE_1024)

#define M 16

#define PIT_MIN_12k8 34
#define PIT_FR2_12k8 128
#define PIT_FR1_12k8 160
#define PIT_MAX_12k8 231

#define PIT_MAX_MAX (PIT_MAX_12k8 + (6 * ((((FAC_FSCALE_MAX * PIT_MIN_12k8) + (FSCALE_DENOM / 2)) / FSCALE_DENOM) - PIT_MIN_12k8)))

#define L_INTERPOL (16 + 1)

#define OPL_DECIM 2

#ifndef PI
#define PI 3.141592654
#endif

#define PREEMPH_FAC 0.68f
#define GAMMA1 0.92f
#define TILT_FAC 0.68f

#define TILT_CODE 0.3f

#define NBITS_MAX (48 * 80 + 46)

#define NBITS_MODE (4 * 2)
#define NBITS_LPC (46)

#define L_TCX 1024

#define NPRM_RE8 (L_TCX + (L_TCX / 8))
#define NPRM_TCX80 (LFAC_1024 + 2 + NPRM_RE8)
#define NPRM_TCX40 (LFAC_1024 + 2 + (NPRM_RE8 / 2))
#define NPRM_TCX20 (LFAC_1024 + 2 + (320 + 320 / 8))

#define NPRM_LPC_NEW 256
#define NPRM_DIV (NPRM_TCX20)

#define L_OLD_SPEECH_HIGH_RATE L_TOTAL_HIGH_RATE - L_FRAME_1024

#define L_FILT 12

#define FRAW 0
#define F3GP 1
#define FWAV 2

#define N_MAX 1152

#define MIN_ACELP_COREMODE 1
#define MAX_ACELP_COREMODE 7

#endif
