
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

#ifndef SAC_NLC_ENC_H__
#define SAC_NLC_ENC_H__

#include "spaceEnclib_const.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bit_buf.h"
#include "sac_types.h"
#include "space_bitstream.h"

typedef enum {

  BACKWARDS = 0x0,
  FORWARDS = 0x1

} DIRECTION;

typedef enum {

  DIFF_FREQ = 0x0,
  DIFF_TIME = 0x1,
  DIFF_PILOT = 0x2

} DIFF_TYPE;

#define CODING_SCHEME_HUFF_1D 0x0u
#define CODING_SCHEME_HUFF_2D 0x1u

typedef enum {

  FREQ_PAIR = 0x0,
  TIME_PAIR = 0x1

} PAIRING;

#define PAIR_SHIFT 4
#define PAIR_MASK 0xf

#define MAXPARAM MAX_NUM_BOXES
#define MAXSETS MAX_NUM_PARAMS
#define MAXBANDS MAX_NUM_BINS

int EcDataPairEnc(HANDLE_BIT_BUF strm,
                  short aaInData[][MAXBANDS],
                  short aHistory[MAXBANDS],
                  DATA_TYPE data_type,
                  int setIdx,
                  int startBand,
                  int dataBands,
                  int coarse_flag,
                  int independency_flag,
                  MPS_MODE mps_mode);

int EcDataSingleEnc(HANDLE_BIT_BUF strm,
                    short aaInData[][MAXBANDS],
                    short aHistory[MAXBANDS],
                    DATA_TYPE data_type,
                    int setIdx,
                    int startBand,
                    int dataBands,
                    int coarse_flag,
                    int independency_flag,
                    MPS_MODE mps_mode);

int EcDataPairEncIPD(HANDLE_BIT_BUF strm,
                     short aaInData[][MAXBANDS],
                     short aHistory[MAXBANDS],
                     DATA_TYPE data_type,
                     int setIdx,
                     int startBand,
                     int dataBands,
                     int pair_flag,
                     int coarse_flag,
                     int independency_flag);

#ifdef __cplusplus
}
#endif

#endif
