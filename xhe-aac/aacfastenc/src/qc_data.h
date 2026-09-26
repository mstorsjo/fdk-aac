
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

#ifndef QC_DATA_H
#define QC_DATA_H

#include "dyn_bits.h"
#include "adj_thr_data.h"
#include "iisSigMap.h"

#define MAX_MODES 25

typedef enum {
  QCDATA_BR_MODE_INVALID = -1,
  QCDATA_BR_MODE_CBR = 0,
  QCDATA_BR_MODE_VBR_1 = 1,
  QCDATA_BR_MODE_VBR_2 = 2,
  QCDATA_BR_MODE_VBR_3 = 3,
  QCDATA_BR_MODE_VBR_4 = 4,
  QCDATA_BR_MODE_VBR_5 = 5,
  QCDATA_BR_MODE_VBR_6 = 6,
  QCDATA_BR_MODE_VBR_0 = 7
} QCDATA_BR_MODE;

typedef enum {
  MUX_RAW = 0,
} TRANS_MUX;

struct QC_INIT {
  CHANNEL_MAPPING* channelMapping;
  int maxBits;
  int averageBits;
  int bitRes;
  TRANS_MUX transMux;
  AACENC_CODEC_TYPE codecType;
  QCDATA_BR_MODE bitrateMode;
  float meanPe;
  int chBitrate;
  int invQuant;
  float maxBitFac;
  int bitrate;
  int useNoiseFilling;
  int useEnhancedNoiseFilling;
  int useLloydMaxQuantizer;

  unsigned short int crcPoly;
  unsigned short int crcStartVal;
};

typedef struct
{
  signed int* quantSpec;
  unsigned int* maxValueInSfb;
  int* scf;
  int globalGain;
  unsigned int noiseLevel;
  int groupingMask;
  unsigned int noiseLevelPrevious;
  SECTION_DATA sectionData;
  int granuleLength;
  int windowShape;

  float* synthTime;
  float reorderRatio;
} QC_OUT_CHANNEL;

typedef struct
{
  int staticBitsUsed;
  int dynBitsUsed;
  float pe;
} QC_OUT_ELEMENT;

typedef struct {
  QC_OUT_CHANNEL* qcChannel[SIGMAP_MAX_SIGNALS];
  QC_OUT_ELEMENT* qcElement[SIGMAP_MAX_ELEMENTS];
  int totStaticBitsUsed;
  int totDynBitsUsed;
  int averageBitsTot;
} QC_OUT;

typedef struct {
  int chBitrate;
  int averageBits;
  int maxBits;
  int bitResLevel;
  int maxBitResBits;
  float relativeBits;
  float relativeBitsVar;
  float chBitDistribution[2];
} ELEMENT_BITS;

typedef struct
{
  int averageBitsTot;
  int globStatBits;
  int nChannels;
  int nElements;
  QCDATA_BR_MODE bitrateMode;

  float vbrQualFactor;
  int invQuant;
  int useLloydMaxQuantizer;
  float maxBitFac;
  int dseBitsLast;
  float forcePE;

  int useNoiseFilling;

  ELEMENT_BITS* elementBits[SIGMAP_MAX_ELEMENTS];
  struct BITCNTR_STATE* hBitCounter;
  ADJ_THR_STATE* hAdjThr;
  int sideInfoTabLong[MAX_SFB_LONG + 1];
  int sideInfoTabShort[MAX_SFB_SHORT + 1];
} QC_STATE;

#endif
