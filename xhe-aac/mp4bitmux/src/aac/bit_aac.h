
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

#ifndef BIT_AAC_H
#define BIT_AAC_H

#include "glob_con.h"
#include "bit_buf.h"
#include "mathlib.h"
#include "iisArithEncoder.h"

enum _si_bis {
  SI_ID_BITS = (3),
  SI_FILL_COUNT_BITS = (4),
  SI_FILL_ESC_COUNT_BITS = (8),
  SI_FILL_EXTENTION_BITS = (4),
  SI_FILL_NIBBLE_BITS = (4),
  SI_SCE_BITS = (4),
  SI_LFE_BITS = (4),
  SI_CPE_BITS = (5),
  SI_CPE_MS_MASK_BITS = (2),
  SI_ICS_INFO_BITS_LONG = (1 + 2 + 1 + 6 + 1),
  SI_ICS_INFO_BITS_SHORT = (1 + 2 + 1 + 4 + (TRANS_FAK - 1)),
  SI_ICS_INFO_BITS_LONG_SCAL = (1 + 2 + 1 + 6),
  SI_ICS_INFO_BITS_SHORT_SCAL = (1 + 2 + 1 + 4 + (TRANS_FAK - 1)),
  SI_ICS_BITS = (8 + 1 + 1 + 1),
  SI_ICS_BITS_ELD = (8 + 1),
  SI_ICS_BITS_NF = 8,
  SI_MS_MASK_ALL_ZERO = 0,
  SI_MS_MASK_SOME = 1,
  SI_MS_MASK_ALL_ONE = 2,
  SI_MS_MASK_CPLX_PRED = 3,
  SI_CCE_ELEMENT_INSTANCE_TAG = 4,
  SI_CCE_IS_SW_CCE_FLAG = 1,
  SI_CCE_NUM_COUPLED_ELEMENTS = 3,
  SI_CCE_CC_TARGET_IS_CPE = 1,
  SI_CCE_CC_TARGET_TAG_SELECT = 4,
  SI_CCE_CC_L = 1,
  SI_CCE_CC_R = 1,
  SI_CCE_CC_DOMAIN = 1,
  SI_CCE_GAIN_ELEMENT_SIGN = 1,
  SI_CCE_GAIN_ELEMENT_SCALE = 2,
  SI_CCE_COMMON_GAIN_ELEMENT_PRESENT = 1,
  SI_USAC_INDEP_FLAG = 1
};

#define SI_SSR_MAX_BAND_BITS 2
#define SI_SSR_ADJUST_NUM_BITS 3
#define SI_SSR_ALEVCODE_BITS 4
#define SI_SSR_ALOCCODE_LONG_BITS 5
#define SI_SSR_ALOCCODE_SHORT_BITS 2
#define SI_SSR_ALOCCODE_STARTA_BITS 4
#define SI_SSR_ALOCCODE_STARTB_BITS 2
#define SI_SSR_ALOCCODE_STOPA_BITS 4
#define SI_SSR_ALOCCODE_STOPB_BITS 5

#define SI_THR_EXTENSION 13
#define SI_NMR_EXTENSION 14

typedef struct bs_scalfac_data {
  int globalGain;
  unsigned int noiseLevel;
  int scalefac[MAX_GROUPED_SFB];
  int isScalefacCoding;
  int isPosition[MAX_GROUPED_SFB];
  int nsScalefacCoding;
  int noiseNrg[MAX_GROUPED_SFB];

} BS_SCALEFAC_DATA;

typedef struct bs_section {
  int codeBook;
  int sfbStart;
  int sfbCnt;
} BS_SECTION;

typedef struct bs_section_data {
  int noOfSections;
  int firstSCF;
  BS_SECTION section[MAX_GROUPED_SFB];
} BS_SECTION_DATA;

typedef struct bs_pitch_data {
  int commonPitch;
  int nBits;
  int nPitches;
  int bActive;
  int pCodedPitchIdx[32];
} BS_PITCH_DATA;

typedef struct bs_ltp_data {
  int ltpDataEnabled;
  int ltpDataPresent;
  int pitchLag;
  int aacSyntax;
  int newLag;
  int gainIndex;
  int ltpMaxSfb;
} BS_LTP_DATA;

typedef struct bs_prediction_data {
  int predictorDataEnabled;
  int predictorDataPresent;
  int predictorReset;
  int predictorResetGroupNumber;
  int predSfbMax;
} BS_PREDICTION_DATA;

typedef struct {
  int startBand;
  int stopBand;
  int order;
  int direction;
  short coefficients[TNS_MAX_ORDER];
} TNS_FILTER;

typedef struct {
  int numOfFilters;
  int coefficientResolution;
  TNS_FILTER filters[TNS_MAX_FILT];
} TNS_SUBBLOCK;

typedef struct bs_tns_data {
  int tnsEnabled;
  int tnsActive;
  int numOfSubblocks;
  int tnsChanMonoLayFromRight;
  TNS_SUBBLOCK subBlock[TRANS_FAK];
  int bCommonTns;
  int bTnsOnLr;
} BS_TNS_DATA;

typedef struct {
  int natks;
  int a_loc[SSR_NATKS];
  int a_idgain[SSR_NATKS];
} SSR_GAIN;

typedef struct bs_ssr_data {
  int ssrEnabled;
  int max_band;
  BLOCK_TYPE blockType;
  SSR_GAIN gain[SSR_NBANDS][TRANS_FAK];
} BS_SSR_DATA;

typedef struct {
  int code;
  int length;
} DIFF_CTRL_ELEM;

typedef struct bs_diffctrl_data {
  int diffCtrlEnabled;
  int nDcGroups;
  DIFF_CTRL_ELEM diffCtrlCode[MAX_GROUPED_SFB];
} BS_DIFFCTRL_DATA;

int encodeNoiseLevel(unsigned int noiseLevel,
                     HANDLE_BIT_BUF hBitStream);

int encodeGlobalGain(BS_SCALEFAC_DATA *scalefacData,
                     int firstSCF,
                     HANDLE_BIT_BUF hBitStream);

int encodeIcsInfo(BLOCK_TYPE blockType,
                  WIN_SHAPE_NGS mdctWindowShape,
                  int groupingMask,
                  int commonWindow,
                  int maxSfb,
                  BS_PREDICTION_DATA *predictionData,
                  BS_LTP_DATA *bs_ltp_data1,
                  BS_LTP_DATA *bs_ltp_data2,
                  JS_FLAG *jsFlag1,
                  JS_FLAG *jsFlag2,
                  HANDLE_BIT_BUF hBitStream,
                  AUDIO_OBJECT_TYPE aot);

int encodeIcsInfoScal(BLOCK_TYPE blockType,
                      WIN_SHAPE_NGS mdctWindowShape,
                      int groupingMask,
                      int maxSfb,
                      HANDLE_BIT_BUF hBitStream);

int encodeReducedIcsInfo(BLOCK_TYPE blockType,
                         int groupingMask,
                         int maxSfb,
                         HANDLE_BIT_BUF hBitStream);

int encodeMSInfo(int sfbCnt,
                 int grpSfb,
                 int maxSfb,
                 JS_FLAG *jsFlag,
                 int maxSfbLastLayer,
                 AUDIO_OBJECT_TYPE aot,
                 int bCplxPredMdct,
                 int *predCoeffRe,
                 int *predCoeffIm,
                 int *predCoeffPrevRe,
                 int *predCoeffPrevIm,
                 int bResetPredictors,
                 int nGroupsPrev,
                 int wSeq,
                 int wSeqPrev,
                 int sfbPerPredBand,
                 int bSwap,
                 int bUsePrevFrame,
                 HANDLE_BIT_BUF hBitStream,
                 int const bUsacIndependenceFlag);

int encodeScaleFactorData(int *sfbBandOffset,
                          BS_SECTION_DATA *sectionData,
                          BS_SCALEFAC_DATA *scalefacData,
                          int *quantSpectrum,
                          AUDIO_OBJECT_TYPE aot,
                          int useNoiseFilling,
                          HANDLE_BIT_BUF hBitStream);

int encodePitchData(BS_PITCH_DATA *bs_pitch_data, HANDLE_BIT_BUF hBitStream);

int encodeTnsData(BS_TNS_DATA *tnsData,
                  AUDIO_OBJECT_TYPE aot,
                  HANDLE_BIT_BUF hBitStream);

int encodeSpectralData(int *sfbBandOffset,
                       BS_SECTION_DATA *sectionData,
                       int *quantSpectrum,
                       HANDLE_BIT_BUF hBitStream);

typedef struct ENC_SPEC_DATA_ARITH *HANDLE_ENC_SPEC_DATA_ARITH;

typedef struct ENC_SPEC_DATA_ARITH2 *HANDLE_ENC_SPEC_DATA_ARITH2;

int createSpectralDataArith(HANDLE_ENC_SPEC_DATA_ARITH *phEncSpecDataArith, int resetDistance);

int createSpectralDataArith2(HANDLE_ENC_SPEC_DATA_ARITH2 *phEncSpecDataArith2, int resetDistance);

int destroySpectralDataArith(HANDLE_ENC_SPEC_DATA_ARITH *phEncSpecDataArith);

int destroySpectralDataArith2(HANDLE_ENC_SPEC_DATA_ARITH2 *phEncSpecDataArith2);

ARIENC_PUBLIC_DATA_HANDLE getArithEncoder(HANDLE_ENC_SPEC_DATA_ARITH2 hEncSpecDataArith2);

int encodeSpectralDataArith(HANDLE_ENC_SPEC_DATA_ARITH hEncSpecDataArith,
                            int *sfbBandOffset,
                            int sfbCnt,
                            int grpSfb,
                            int maxSfbPerGroup,
                            int groupingMask,
                            BLOCK_TYPE blockType,
                            int *quantSpectrum,
                            HANDLE_BIT_BUF hBitStream);

int encodeSpectralDataArith2(HANDLE_ENC_SPEC_DATA_ARITH2 hEncSpecDataArith2,
                             int const *sfbBandOffset,
                             int sfbCnt,
                             int grpSfb,
                             int maxSfbPerGroup,
                             int groupingMask,
                             BLOCK_TYPE blockType,
                             int const *quantSpectrum,
                             int bUsacIndepFlag,
                             HANDLE_BIT_BUF hBitStream);

void resetSpectralDataArithFrameCount(HANDLE_ENC_SPEC_DATA_ARITH hEncSpecDataArith);

void resetSpectralDataArithFrameCount2(HANDLE_ENC_SPEC_DATA_ARITH2 hEncSpecDataArith);

int encodeCplxPredData(int sfbCnt,
                       int grpSfb,
                       int maxSfb,
                       const int *jsFlag,
                       const int *predCoeffRe, const int *predCoeffIm,
                       int *predCoeffPrevRe, int *predCoeffPrevIm,
                       int bResetPredictors, int nGroupsPrev,
                       BLOCK_TYPE windowSequence, BLOCK_TYPE windowSequencePrev,
                       int sfbPerPredBand,
                       int bUsePrevFrame,
                       HANDLE_BIT_BUF hBitStream,
                       int const bUsacIndependenceFlag);

int countBitsScf(int delta);

int getSpectralDataArith2(
    HANDLE_ENC_SPEC_DATA_ARITH2 hEncSpecDataArith2,
    HANDLE_BIT_BUF hBitBuf,
    unsigned int const bUsacIndepFlag);
#endif
