
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

#ifndef SBR_DEF_H
#define SBR_DEF_H

#define SWAP(a, b) tempr = a, a = b, b = tempr
#define TRUE 1
#define FALSE 0

#define SBR_QMF_CHANNELS 64
#define SBR_QMF_PROTOFILT_LENGTH 640
#define SBR_QMF_SUBSAMPLE_DELAY ((int)(SBR_QMF_PROTOFILT_LENGTH / SBR_QMF_CHANNELS + 1) / 2)

typedef enum {
  INVALID = 0,
  NO_RESAMPLING = 1,
  RESAMPLE_BY_2 = 2,
  RESAMPLE_BY_4 = 4,
  RESAMPLE_BY_8 = 8
} SBR_RESAMPLING;

#define SBR_PCM_LEVEL 1.0f
#define NORM_SBR_PCM_LEVEL (SBR_PCM_LEVEL / 32768.0f)
#define INORM_SBR_PCM_LEVEL (1.0f / NORM_SBR_PCM_LEVEL)
#define NORM_SBR_PCM_LEVEL_SQ (SBR_PCM_LEVEL / (32768.0f * 32768.0f))
#define INORM_SBR_PCM_LEVEL_SQ (1.0f / NORM_SBR_PCM_LEVEL_SQ)

#define PI 3.14159265358978f
#define EPS 1e-12
#define EPS_NS (EPS * NORM_SBR_PCM_LEVEL_SQ)

#define LOG2 0.69314718056f
#define ILOG2 1.442695041f

#define RELAXATION 1e-6f

#define SBR_COMP_MODE_DELTA 0
#define SBR_COMP_MODE_CTS 1
#define SBR_MAX_ENERGY_VALUES 5
#define SBR_GLOBAL_TONALITY_VALUES 3

#define MAX_NOISE_ENVELOPES 2
#define MAX_NUM_NOISE_VALUES (MAX_NUM_NOISE_COEFFS * MAX_NOISE_ENVELOPES)

#define MAX_NUM_ENVELOPE_VALUES 448
#define MAX_ENVELOPES 16

#define MAX_FREQ_COEFFS_SBR_RATIO_16_64 56

#define MAX_FREQ_COEFFS_SBR_RATIO_16_64_FS44100 56
#define MAX_FREQ_COEFFS_SBR_RATIO_16_64_FS48000 56

#define MAX_FREQ_COEFFS 48

#define MAX_FREQ_COEFFS_FS32000 48
#define MAX_FREQ_COEFFS_FS44100 35
#define MAX_FREQ_COEFFS_FS48000 32

#define NOISE_FLOOR_OFFSET 6

#define LENGTH_SBR_FRAME_INFO 35

#define SBR_NSFB_LOW_RES 9
#define SBR_NSFB_HIGH_RES 18

#define SBR_XPOS_CTRL_DEFAULT 2

#define SBR_FREQ_SCALE_DEFAULT 2
#define SBR_ALTER_SCALE_DEFAULT 1
#define SBR_NOISE_BANDS_DEFAULT 2

#define SBR_LIMITER_BANDS_DEFAULT 2
#define SBR_LIMITER_GAINS_DEFAULT 2
#define SBR_LIMITER_GAINS_INFINITE 3
#define SBR_INTERPOL_FREQ_DEFAULT 1

#define SBR_SMOOTHING_LENGTH_DEFAULT 1

#define SI_SBR_PRE_FLAT_BITS 1
#define SI_SBR_RESERVED_BITS_HDR 2
#define SI_SBR_RESERVED_BITS_DATA 4
#define SI_SBR_DATA_EXTRA_BITS 1

#define SI_SBR_AMP_RES_BITS 1
#define SI_SBR_COUPLING_BITS 1
#define SI_SBR_START_FREQ_BITS 4
#define SI_SBR_STOP_FREQ_BITS 4

#define SI_SBR_XOVER_BAND_BITS 3
#define SI_SBR_XOVER_BAND_BITS_SAAC 4

#define SI_SBR_HEADER_EXTRA_1_BITS 1
#define SI_SBR_HEADER_EXTRA_2_BITS 1

#define SI_SBR_FREQ_SCALE_BITS 2
#define SI_SBR_ALTER_SCALE_BITS 1
#define SI_SBR_NOISE_BANDS_BITS 2

#define SI_SBR_LIMITER_BANDS_BITS 2
#define SI_SBR_LIMITER_GAINS_BITS 2
#define SI_SBR_INTERPOL_FREQ_BITS 1
#define SI_SBR_SMOOTHING_LENGTH_BITS 1

#define SBR_CLA_BITS 2
#define SBR_ENV_BITS 2
#define SBR_ABS_BITS 3
#define SBR_ABS_BITS_AAC 2
#define SBR_NUM_BITS 2
#define SBR_REL_BITS 2
#define SBR_RES_BITS 1
#define SBR_DIR_BITS 1

#define SI_SBR_OVERSAMPLING_BITS 1
#define SI_SBR_INVF_MODE_BITS 2
#define SI_SBR_PITCHFLAG_BITS 1
#define SI_SBR_PITCHESTIMATE_BITS 7

#define SI_SBR_START_ENV_BITS_AMP_RES_3_0 6
#define SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_3_0 5
#define SI_SBR_START_NOISE_BITS_AMP_RES_3_0 5
#define SI_SBR_START_NOISE_BITS_BALANCE_AMP_RES_3_0 5

#define SI_SBR_START_ENV_BITS_AMP_RES_1_5 7
#define SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_1_5 6

#define SI_SBR_EXTENDED_DATA_BITS 1
#define SI_SBR_EXTENSION_SIZE_BITS 4
#define SI_SBR_EXTENSION_ESC_COUNT_BITS 8
#define SI_SBR_EXTENSION_ID_BITS 2

#define SBR_EXTENDED_DATA_MAX_CNT (15 + 255)

#define SI_SBR_TEMP_SHAPE_BITS 1
#define SI_SBR_INTER_TEMP_SHAPE_MODE_BITS 2

#define SI_SBR_MODE_BIT 1

#define SI_SBR_INFO_PRESENT_BITS 1
#define SI_SBR_USE_DFLT_HEADER_BITS 1

#define FREQ 0
#define TIME 1

#define CODE_BOOK_SCF_LAV00 60
#define CODE_BOOK_SCF_LAV01 31
#define CODE_BOOK_SCF_LAV10 60
#define CODE_BOOK_SCF_LAV11 31
#define CODE_BOOK_SCF_LAV_BALANCE11 12
#define CODE_BOOK_SCF_LAV_BALANCE10 24

#endif
