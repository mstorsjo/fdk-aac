
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

#include "IIS_MP4AEnc_ASC_Interface.h"
#include "audiospecificconfig.h"
#include "bit_buf.h"

typedef enum {
  syncExtensionInvalid = 0,
  syncExtensionRESERVED = 0xFFF
} syncExtension;

#define SAMPLING_FREQUENCY_INDEX_ESCAPE_VALUE (0xF)

typedef struct {
  unsigned int samplingFrequency;
  unsigned int samplingFrequencyIndex;
} SAMPLING_FREQUENCY_INDEX_INFO;

static const SAMPLING_FREQUENCY_INDEX_INFO samplingFrequencyIndexInfoTable[] =
    {
        {96000, 0},
        {88200, 1},
        {64000, 2},
        {48000, 3},
        {44100, 4},
        {32000, 5},
        {24000, 6},
        {22050, 7},
        {16000, 8},
        {12000, 9},
        {11025, 10},
        {8000, 11},
        {7350, 12}};

static unsigned int getSamplingFrequencyIndex(unsigned int samplingFrequency) {
  unsigned int n;

  for (n = 0; n < sizeof(samplingFrequencyIndexInfoTable) / sizeof(samplingFrequencyIndexInfoTable[0]); n++) {
    SAMPLING_FREQUENCY_INDEX_INFO const* const pSamplingFrequencyIndexInfo = &samplingFrequencyIndexInfoTable[n];

    if (pSamplingFrequencyIndexInfo->samplingFrequency == samplingFrequency) {
      return pSamplingFrequencyIndexInfo->samplingFrequencyIndex;
    }
  }

  return SAMPLING_FREQUENCY_INDEX_ESCAPE_VALUE;
}

static void writeAOT(AUDIO_OBJECT_TYPE aot, HANDLE_BITBUFFER hBitBuf) {
  if (aot < AOT_ESCAPE)
    WriteBits((HANDLE_BIT_BUF)hBitBuf, (int)aot, 5);
  else {
    int tmpAot = (int)aot - (int)AOT_ESCAPE - 1;
    WriteBits((HANDLE_BIT_BUF)hBitBuf, (int)AOT_ESCAPE, 5);
    WriteBits((HANDLE_BIT_BUF)hBitBuf, tmpAot, 6);
  }
}

HANDLE_ERROR_INFO
WriteSampleRateData(unsigned int samplingFrequency, HANDLE_BITBUFFER hBitBuf) {
  unsigned int samplingFrequencyIndex = getSamplingFrequencyIndex(samplingFrequency);
  WriteBits((HANDLE_BIT_BUF)hBitBuf, (int)samplingFrequencyIndex, 4);

  if (samplingFrequencyIndex == SAMPLING_FREQUENCY_INDEX_ESCAPE_VALUE) {
    WriteBits((HANDLE_BIT_BUF)hBitBuf, (int)samplingFrequency, 24);
  }
  return noError;
}

HANDLE_ERROR_INFO
WriteAudioSpecificConfig(HANDLE_ASC hAsc, HANDLE_BITBUFFER hDestBitBuf) {
  HANDLE_ERROR_INFO errorInfo = noError;
  HANDLE_BIT_BUF hBitBuf = NULL;

  unsigned int samplingFrequencyIndex;
  unsigned int aot = 0;

  if (hDestBitBuf == NULL) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle hDestBitBuf.");
  }

  if (hAsc == NULL) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle hAsc.");
  }

  if (errorInfo == noError) {
    if (0 == hAsc->channelConfiguration) {
      hBitBuf = CreateBitBuffer(ASC_SIZE * 8);

      if (hBitBuf == NULL) {
        errorInfo = iisUtil_ERROR(CDI, "Out of Memory.");
      }
    } else {
      hBitBuf = (HANDLE_BIT_BUF)hDestBitBuf;
    }
  }

  if (errorInfo == noError) {
    switch (hAsc->aot) {
      case AOT_MP2_AAC_MAIN:
        aot = 1;
        break;
      case AOT_MP2_AAC_LC:
        aot = 2;
        break;
      case AOT_MP2_AAC_SSR:
        aot = 3;
        break;
      default: {
        aot = hAsc->aot;
      } break;
    }
  }

  if (errorInfo == noError) {
    writeAOT(aot, hBitBuf);
    samplingFrequencyIndex = getSamplingFrequencyIndex(hAsc->samplingFrequency);
    WriteBits((HANDLE_BIT_BUF)hBitBuf, (int)samplingFrequencyIndex, 4);

    if (samplingFrequencyIndex == SAMPLING_FREQUENCY_INDEX_ESCAPE_VALUE) {
      WriteBits((HANDLE_BIT_BUF)hBitBuf, (int)hAsc->samplingFrequency, 24);
    }

    WriteBits((HANDLE_BIT_BUF)hBitBuf, (int)hAsc->channelConfiguration, 4);
  }

  if (errorInfo == noError) {
    if (aot == AOT_MPEGS) {
      WriteBits((HANDLE_BIT_BUF)hBitBuf, hAsc->sacPayloadEmbedding, 1);
    }
  }

  if (errorInfo == noError) {
    if (hAsc->hAotSpecificPart.WriteAotSpecificConfig != NULL) {
      hAsc->hAotSpecificPart.WriteAotSpecificConfig((struct AUDIO_SPECIFIC_CONFIG*)hAsc,
                                                    hAsc->hAotSpecificPart.aotSpecificConfig,
                                                    (HANDLE_BIT_BUF)hBitBuf);
    }
  }

  if (errorInfo == noError) {
    if ((aot == AOT_ER_AAC_LC) || (aot == AOT_ER_AAC_LTP) ||
        (aot == AOT_ER_AAC_SCAL) || (aot == AOT_ER_TWIN_VQ) ||
        (aot == AOT_ER_BSAC) || (aot == AOT_ER_AAC_LD) ||
        (aot == AOT_ER_CELP) || (aot == AOT_ER_HVXC) ||
        (aot == AOT_ER_PARA) || (aot == AOT_ER_HILN) ||
        (aot == AOT_ER_AAC_ELD)) {
      WriteBits((HANDLE_BIT_BUF)hBitBuf, (int)hAsc->epConfig, 2);
    }
  }

  if (errorInfo == noError) {
    if (hBitBuf != hDestBitBuf) {
      ngsCopyBits((HANDLE_BIT_BUF)hDestBitBuf, hBitBuf, GetBitsAvail(hBitBuf));
    }
  }
  if (hBitBuf && (hBitBuf != hDestBitBuf)) {
    DeleteBitBuffer(hBitBuf);
  }

  return errorInfo;
}
