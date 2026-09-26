
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

#ifndef lpd_wrapper_h_
#define lpd_wrapper_h_

#include "aacenc_internal.h"
#include "bit_enc.h"
#include "iisBitFrame.h"
#include "qc_data.h"
#include "extpayload.h"
#include "iisSigMap.h"
#include "psy_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LPD_WRAPPER_NO_ERROR,
  LPD_WRAPPER_UNKNOWN_ERROR,
  LPD_WRAPPER_INIT_ERROR,
  LPD_WRAPPER_ILLEGAL_PARAMETER,
  LPD_WRAPPER_INVALID_POINTER
} LPD_WRAPPER_ERROR;

typedef enum lpd_wrapper_ipf_state {
  LPD_WRAPPER_IPF_STATE_NO,
  LPD_WRAPPER_IPF_STATE_RAP_FIRST_PREROLL,
  LPD_WRAPPER_IPF_STATE_RAP_NEXT_PREROLL,
  LPD_WRAPPER_IPF_STATE_RAP_IPF,
  LPD_WRAPPER_IPF_STATE_RAP_IPF_PREROLL,
  LPD_WRAPPER_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL,
  LPD_WRAPPER_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL,
  LPD_WRAPPER_IPF_STATE_CONFIGCHANGE_IPF
} LPD_WRAPPER_IPF_STATE;

typedef enum lpd_wrapper_coding_mode {
  LPD_WRAPPER_CODING_MODE_INVALID = -1,
  LPD_WRAPPER_CODING_MODE_SWITCHED = 0,
  LPD_WRAPPER_CODING_MODE_ACELP,
  LPD_WRAPPER_CODING_MODE_TCX
} LPD_WRAPPER_CODING_MODE;

typedef enum lpd_wrapper_codec_type {
  LPD_WRAPPER_CODEC_AAC = 0,
  LPD_WRAPPER_CODEC_XHEAAC = 1,
  LPD_WRAPPER_CODEC_MPEGH = 2
} LPD_WRAPPER_CODEC_TYPE;

typedef struct lpdWrapper_setup_tag {
  int nChannels;
  int nElements;
  ELEMENT_INFO_HANDLE phElInfo[SIGMAP_MAX_ELEMENTS];
  LPD_WRAPPER_CODEC_TYPE codecType;
  int sampleRate;
  int acelpModeIndex;
  int totalBitRate;
  int isVbr;
  int nGranuleLength;
  LPD_WRAPPER_CODING_MODE codingMode;
  int optimizedSpeedPulseSearch;
  int useNoiseFilling;
} LPD_WRAPPER_SETUP, *LPD_WRAPPER_SETUP_HANDLE;

typedef struct lpdWrapper_tag LPD_WRAPPER, *LPD_WRAPPER_HANDLE;

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_open(
    LPD_WRAPPER_HANDLE *phLpdWrapper,
    LPD_WRAPPER_SETUP_HANDLE hLpdWrapperSetup);

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_process(
    LPD_WRAPPER_HANDLE hLpdWrapper,
    int bitReservoir,
    int bitReservoirMax,
    HANDLE_BITSTREAM_ENC hBsEnc,
    PSY_DATA *psyData[],
    int el,
    INTERN_CORE_MODE *coreMode,
    INTERN_CORE_MODE *coreModePrev,
    INTERN_CORE_MODE *coreModeNext,
    int maxBitsToUse,
    const int bUsacIndepFlag,
    const LPD_WRAPPER_IPF_STATE ipfState);

LPD_WRAPPER_ERROR iisaacfenc_wrap_fac_process(
    LPD_WRAPPER_HANDLE hLpdWrapper,
    int ch0,
    int ch,
    PSY_OUT_CHANNEL *psyOutChannel,
    QC_OUT_CHANNEL *qcOutChannel,
    INTERN_CORE_MODE coreModePrev,
    INTERN_CORE_MODE coreModeNext,
    char *facPrm,
    int *Nbits_fac);

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_set_restricted_mode(
    LPD_WRAPPER_HANDLE hLpdWrapper,
    int ch,
    int mode);

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_transfer_acelp_data(
    LPD_WRAPPER_HANDLE hLpdWrapper,
    int ch,
    unsigned char *acelpData,
    int *acelpDataBitCnt);

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_transfer_fac_data(
    LPD_WRAPPER_HANDLE hLpdWrapper,
    int ch,
    unsigned char *facData,
    int *facDataBitCnt);

int iisaacfenc_wrap_lpd_last_sub_frame_was_lpd(
    LPD_WRAPPER_HANDLE hLpdWrapper,
    int ch);

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_close(
    LPD_WRAPPER_HANDLE hLpdWrapper);

LPD_WRAPPER_ERROR iisaacfenc_calculate_max_bits_to_use(
    LPD_WRAPPER_HANDLE hLpdWrapper,
    HANDLE_EXTPAYLOAD_CONTAINER *hExtContainer,
    int numExtContainer,
    int el,
    unsigned int nBitsTransportOverhead,
    int maximumNumberOfBitsForThisFrame,
    unsigned int *maxBitsToUse);

#ifdef __cplusplus
}
#endif

#endif
