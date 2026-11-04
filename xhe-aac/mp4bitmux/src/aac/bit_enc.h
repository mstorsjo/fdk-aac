
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

#ifndef BIT_ENC_H
#define BIT_ENC_H

struct BSENC;
typedef struct BSENC *HANDLE_BITSTREAM_ENC;

#include "glob_con.h"
#include "iisutillib.h"
#include "bit_buf.h"
#include "iisBitFrame.h"
#include "bit_aac.h"
#include "bs_configuration.h"
#include "extpayload.h"

#define MAX_BIT_ENC_CHANNELS 56
#define MX_BITSTREAM_SIZE 64000

typedef struct {
  int channelNdx;
} BS_SINGLE_CHANNEL_ELEMENT;

typedef struct {
  int channelNdx;
  int independentFlag;
  int ccDomain;
  int gainElementSign;
  int gainElementScale;
  int numberOfCoupledElements;
  struct bsCouplingInfo *coupledWith;
} BS_COUPLING_CHANNEL_ELEMENT;

typedef struct {
  int nChannels;
  int channelNdx[2];
} BS_CHANNEL_PAIR_ELEMENT;

typedef struct {
  int channelNdx;
} BS_LFE_CHANNEL_ELEMENT;

typedef struct {
  int is_drc;
  int data_byte_align_flag;
  int byteCount;
  unsigned char data_stream_byte[512];
} BSE_DATA_STREAM_ELEMENT;

typedef struct {
  ELEMENT_TYPE elementType;
  int instanceTag;

  union {
    BS_CHANNEL_PAIR_ELEMENT cpe;
    BS_SINGLE_CHANNEL_ELEMENT sce;
    BS_COUPLING_CHANNEL_ELEMENT cce;
    BS_LFE_CHANNEL_ELEMENT lfe;
    BSE_DATA_STREAM_ELEMENT dse;
  } element;
} BS_MAPPING_ELEMENT;

typedef struct bsCouplingInfo {
  int commonGain[2];
  BS_MAPPING_ELEMENT *mappingElement;
} BS_COUPLING_INFO;

typedef struct {
  int noOfChannels;
  int noOfEffectiveChannels;
  int noOfElements;
  BS_MAPPING_ELEMENT **element;
} BS_CHANNEL_MAPPING;

typedef struct {
  int extEleFil_present;
  int extEleFil_numBits;
  int extEleFil_posBgn;
  int extEleFil_posEnd;
} BS_EXT_ELE_FIL_INFO;

typedef struct {
  int bitstream;
  FILE *file;
  int nmr;
  int framecounter;
} WRITE_THRESHOLD;

typedef enum {
  BS_BR_MODE_INVALID = -1,
  BS_BR_MODE_DEFAULT = 0,
  BS_BR_MODE_DMB = 3
} BS_BR_MODE;

typedef enum {
  BITSTREAM_MODE_SIMULATE = 0,
  BITSTREAM_MODE_CODING = 1
} BITSTREAM_MODE;

typedef struct {
  STREAM_TYPE streamType;
  AUDIO_OBJECT_TYPE aot;
  int sampleRate;
  int sampleRateIndex;
  BS_CHANNEL_MAPPING channelMapping;
  int ancDataBits;
  int nChannels;
  WRITE_THRESHOLD writeThr;
  int variableBitrate;
  int useNoPadding;
  float bitResInitFillLevel;
  float bitResDistribution;
  int timeWarpedMdct;
  int useNoiseFilling;
  BS_BR_MODE bitrateMode;
  int useEnhancedNoiseFilling;
  int igfUseHighRes;
  int igfAfterTnsSynth;
  int elementLengthPresent;
} BIT_ENC_CONFIG;

typedef struct {
  BLOCK_TYPE blockType[MAX_BIT_ENC_CHANNELS];
  int totalFrameBits;
  int totHeaderBits;
  int availableFrameBits;
  int availableDynpartBits;
  int totDynBits;
  int sideInfoBits;
  int dseBits;
  int additionalElemBits;
  int elemSideInfoBits[MAX_BIT_ENC_CHANNELS];
  int totFillBits;
  int extraFillBytes;
  int fillElementBits;
  int alignBits;
  int outputframesize;
  int minFrameBytes;
} BIT_ENC_FRAME_DATA;

struct ENC_SPEC_DATA_ARITH2;

typedef struct {
  BLOCK_TYPE blockType;
  WIN_SHAPE_NGS mdctWindowShape;
  int windowKernel;
  int commonWindow;
  int sfbCnt;
  int grpSfb;
  int maxSfb;
  int groupingMask;
  int sfbBandOffset[MAX_GROUPED_SFB + 1];
  JS_FLAG jsFlag[MAX_GROUPED_SFB];
  BS_LTP_DATA bs_ltp_data;
  BS_PREDICTION_DATA bs_prediction_data;
  BS_PITCH_DATA bs_pitch_data;
  int timeWarpedMdct;

  BS_TNS_DATA bs_tns_data;
  BS_SSR_DATA bs_ssr_data;

  BS_SCALEFAC_DATA bs_scalefac_data;
  BS_SECTION_DATA bs_section_data;
  BS_DIFFCTRL_DATA bs_diffctrl_data;
  BS_DIFFCTRL_DATA bs_diffctrlLR_data;

  int aQuantSpectrum[MAX_GRANULE_LEN];

  int acelpDataBitCnt;
  unsigned char acelpData[512];

  int FacDataBitCnt;
  unsigned char FacData[1000];

  struct ENC_SPEC_DATA_ARITH2 *hEncSpecDataArith;
  float reorderRatio;
  int remapFlag;

  float xmin[MAX_GROUPED_SFB];
  float xfsf[MAX_GROUPED_SFB];
  float nmr[MAX_GROUPED_SFB];

  int bCplxPredMdct;
  int predCoefRe[MAX_GROUPED_SFB];
  int predCoefIm[MAX_GROUPED_SFB];
  int predCoefPrevRe[MAX_GROUPED_SFB];
  int predCoefPrevIm[MAX_GROUPED_SFB];
  int bResetPredictors;
  int nGroupsPrev;
  int windowSequencePrev;
  int sfbPerPredBand;
  int bSwappedChannel;
  int bPrevFrame;
  int ltpPostCommon;
  int ltpPostActive;
  int ltpPostPitchLag;
  int ltpPostGain;

} BIT_ENC_CHANNEL_DATA;

typedef BIT_ENC_CHANNEL_DATA BIT_ENC_CHANNEL_DATA_ARR[MAX_BIT_ENC_CHANNELS];

typedef enum {
  USAC_INDEPENDENCE_FLAG = 0x0001,
  RAP_FRAME = 0x0002
} FRAME_STATE;

struct BSENC {
  int superFrameConfig;
  int DataReadyToWrite;
  BIT_ENC_CONFIG config;
  BIT_ENC_FRAME_DATA frameData;

  HANDLE_BIT_BUF mxBitstream;
  HANDLE_BIT_BUF mxHuffCodeBookBuffer;
  BIT_ENC_CHANNEL_DATA bitEncChannelData[MAX_BIT_ENC_CHANNELS];

  BS_EXT_ELE_FIL_INFO extEleFilInfo;
  unsigned int usacByteAlignmentBits;
};

extern HANDLE_ERROR_INFO
CreateBitstreamEncoder(HANDLE_BITSTREAM_ENC *bsEnc,
                       BS_CONFIGURATION config,
                       const int sampleRate,
                       const BS_CHANNEL_MAPPING *channelMapping,
                       const STREAM_TYPE streamType,
                       const AUDIO_OBJECT_TYPE aot);

extern HANDLE_ERROR_INFO
AdvanceBitstreamEncoder(HANDLE_BITSTREAM_ENC bsEnc,
                        const int additionalBits,
                        const int *additionalElemBits,
                        const int externalElemBits,
                        int headerBits,
                        int totalVarFrameBits,
                        const int *coreMode,
                        const int bUsacIndependenceFlag,
                        IISBITFRAME_HANDLE hBitFrame,
                        int isVbr);

extern HANDLE_ERROR_INFO
WriteBitstream(HANDLE_BITSTREAM_ENC bsEnc,
               const int *additionalElemBits,
               const unsigned char *const *additionalElemData,
               const int *saasSbrBits,
               const unsigned char **saasSbrPayload,
               int *bitstreamOutBytes,
               HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
               int *coreMode,
               const int bUsacIndepFlag,
               HANDLE_EXTPAYLOAD_CONTAINER *hExternalContainer,
               int numExternalContainersInUse);

extern HANDLE_ERROR_INFO WriteBitstreamSimulationMode(
    HANDLE_BITSTREAM_ENC bsEnc,
    const int *additionalElemBits,
    unsigned char const *const *additionalElemData,
    const int *saasSbrBits,
    const unsigned char **saasSbrPayload,
    int *bitstreamOutBytes,
    HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
    int *coreMode,
    const int bUsacIndepFlag,
    HANDLE_EXTPAYLOAD_CONTAINER *hExternalContainer,
    int numExternalContainersInUse,
    int *minBitsForValidBitstream

);

extern void
DeleteBitstreamEncoder(HANDLE_BITSTREAM_ENC bsEnc);

extern float
GetBitresFullness(HANDLE_BITSTREAM_ENC bsEnc);

extern HANDLE_BIT_BUF
bsencGetBitstream(HANDLE_BITSTREAM_ENC bsEnc);

void InitDefaultBitstreamConfiguration(BS_CONFIGURATION *config);

#endif
