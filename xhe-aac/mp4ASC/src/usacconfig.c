
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

#include "usacconfig.h"
#include "bit_buf.h"

static const int UsacSamplingFrequencyTable[] =
    {
        96000,
        88200,
        64000,
        48000,
        44100,
        32000,
        24000,
        22050,
        16000,
        12000,
        11025,
        8000,
        7350,
        -1,
        -1,
        57600,
        51200,
        40000,
        38400,
        34150,
        28800,
        25600,
        20000,
        19200,
        17075,
        14400,
        12800,
        9600,
        -1,
        -1,
        -1,
        0,
};

static unsigned int getUsacSamplingFrequencyIndex(unsigned int samplingFrequency) {
  unsigned int sfIndex;

  for (sfIndex = 0; sfIndex < 0x1f; sfIndex++) {
    if (UsacSamplingFrequencyTable[sfIndex] == (int)samplingFrequency) break;
  }

  return sfIndex;
}

static int getCoreSbrFrameLengthIndex(USAC_CONFIG_OUT_FRAMLENGTH outFrameLength, USAC_CONFIG_SBR_RATIO_INDEX sbrRatioIndex) {
  int index = -1;

  switch (outFrameLength) {
    case USAC_CONFIG_OUT_FRAMELENGTH_768:
      if (sbrRatioIndex == USAC_CONFIG_SBR_RATIO_INDEX_NO_SBR) {
        index = 0;
      }
      break;
    case USAC_CONFIG_OUT_FRAMELENGTH_1024:
      if (sbrRatioIndex == USAC_CONFIG_SBR_RATIO_INDEX_NO_SBR) {
        index = 1;
      }
      break;
    case USAC_CONFIG_OUT_FRAMELENGTH_2048:
      if (sbrRatioIndex == USAC_CONFIG_SBR_RATIO_INDEX_8_3) {
        index = 2;
      } else if (sbrRatioIndex == USAC_CONFIG_SBR_RATIO_INDEX_2_1) {
        index = 3;
      }
      break;
    case USAC_CONFIG_OUT_FRAMELENGTH_4096:
      if (sbrRatioIndex == USAC_CONFIG_SBR_RATIO_INDEX_4_1) {
        index = 4;
      }
      break;
    default:
      break;
  }

  return index;
}

static void writeEscapedValue(HANDLE_BIT_BUF hBitBuf, unsigned int value, unsigned int nBits1, unsigned int nBits2, unsigned int nBits3) {
  const unsigned int maxValue1 = (1 << nBits1) - 1;
  const unsigned int maxValue2 = (1 << nBits2) - 1;
  const unsigned int maxValue3 = (1 << nBits3) - 1;
  unsigned int valueLeft = value;
  unsigned int valueToWrite;

  valueToWrite = min(valueLeft, maxValue1);
  WriteBits(hBitBuf, valueToWrite, nBits1);

  if (valueToWrite == maxValue1) {
    valueLeft = valueLeft - valueToWrite;

    valueToWrite = min(valueLeft, maxValue2);
    WriteBits(hBitBuf, valueToWrite, nBits2);

    if (valueToWrite == maxValue2) {
      valueLeft = valueLeft - valueToWrite;

      valueToWrite = min(valueLeft, maxValue3);
      WriteBits(hBitBuf, valueToWrite, nBits3);
      assert((valueLeft - valueToWrite) == 0);
    }
  }

  return;
}

static void WriteUsacCoreConfig(USAC_CORE_CONFIG *pUsacCoreConfig, HANDLE_BIT_BUF hBitBuf) {
  pUsacCoreConfig->tw_mdct = 0;

  WriteBits(hBitBuf, pUsacCoreConfig->tw_mdct, 1);
  WriteBits(hBitBuf, pUsacCoreConfig->noiseFilling, 1);

  return;
}

static void WriteUsacSbrHeader(USAC_SBR_HEADER *pUsacSbrHeader, HANDLE_BIT_BUF hBitBuf) {
  WriteBits(hBitBuf, pUsacSbrHeader->start_freq, 4);
  WriteBits(hBitBuf, pUsacSbrHeader->stop_freq, 4);
  WriteBits(hBitBuf, pUsacSbrHeader->header_extra1, 1);
  WriteBits(hBitBuf, pUsacSbrHeader->header_extra2, 1);

  if (pUsacSbrHeader->header_extra1) {
    WriteBits(hBitBuf, pUsacSbrHeader->freq_scale, 2);
    WriteBits(hBitBuf, pUsacSbrHeader->alter_scale, 1);
    WriteBits(hBitBuf, pUsacSbrHeader->noise_bands, 2);
  }

  if (pUsacSbrHeader->header_extra2) {
    WriteBits(hBitBuf, pUsacSbrHeader->limiter_bands, 2);
    WriteBits(hBitBuf, pUsacSbrHeader->limiter_gains, 2);
    WriteBits(hBitBuf, pUsacSbrHeader->interpol_freq, 1);
    WriteBits(hBitBuf, pUsacSbrHeader->smoothing_mode, 1);
  }

  return;
}

static void WriteUsacSbrConfig(USAC_SBR_CONFIG *pUsacSbrConfig, HANDLE_BIT_BUF hBitBuf) {
  WriteBits(hBitBuf, pUsacSbrConfig->harmonicSBR, 1);
  WriteBits(hBitBuf, pUsacSbrConfig->bs_interTes, 1);
  WriteBits(hBitBuf, pUsacSbrConfig->bs_pvc, 1);

  WriteUsacSbrHeader(&(pUsacSbrConfig->sbrDfltHeader), hBitBuf);

  return;
}

static void WriteSceConfig(USAC_SCE_CONFIG *pUsacSceConfig, USAC_CONFIG_SBR_RATIO_INDEX sbrRatioIndex, HANDLE_BIT_BUF hBitBuf) {
  WriteUsacCoreConfig(&(pUsacSceConfig->usacCoreConfig), hBitBuf);

  if (sbrRatioIndex > 0) {
    WriteUsacSbrConfig(&(pUsacSceConfig->usacSbrConfig), hBitBuf);
  }

  return;
}

static void WriteMps212Config(USAC_MPS212_CONFIG *pUsacMps212Config, unsigned int bsResidualCoding, HANDLE_BIT_BUF hBitBuf) {
  WriteBits(hBitBuf, pUsacMps212Config->bsFreqRes, 3);
  WriteBits(hBitBuf, pUsacMps212Config->bsFixedGainDMX, 3);
  WriteBits(hBitBuf, pUsacMps212Config->bsTempShapeConfig, 2);
  WriteBits(hBitBuf, pUsacMps212Config->bsDecorrConfig, 2);
  WriteBits(hBitBuf, pUsacMps212Config->bsHighRateMode, 1);
  WriteBits(hBitBuf, pUsacMps212Config->bsPhaseCoding, 1);
  WriteBits(hBitBuf, pUsacMps212Config->bsOttBandsPhasePresent, 1);

  if (pUsacMps212Config->bsOttBandsPhasePresent) {
    WriteBits(hBitBuf, pUsacMps212Config->bsOttBandsPhase, 5);
  }

  if (bsResidualCoding) {
    WriteBits(hBitBuf, pUsacMps212Config->bsResidualBands, 5);
    WriteBits(hBitBuf, pUsacMps212Config->bsPseudoLr, 1);
  }

  if (pUsacMps212Config->bsTempShapeConfig == 2) {
    WriteBits(hBitBuf, pUsacMps212Config->bsEnvQuantMode, 1);
  }

  return;
}

static void WriteCpeConfig(USAC_CPE_CONFIG *pUsacCpeConfig, USAC_CONFIG_SBR_RATIO_INDEX sbrRatioIndex, HANDLE_BIT_BUF hBitBuf) {
  WriteUsacCoreConfig(&(pUsacCpeConfig->usacCoreConfig), hBitBuf);

  if (sbrRatioIndex > 0) {
    WriteUsacSbrConfig(&(pUsacCpeConfig->usacSbrConfig), hBitBuf);
    WriteBits(hBitBuf, pUsacCpeConfig->stereoConfigIndex, 2);
  }

  if (pUsacCpeConfig->stereoConfigIndex) {
    int bsResidualCoding = (pUsacCpeConfig->stereoConfigIndex > 1) ? 1 : 0;
    assert(sbrRatioIndex > 0);

    WriteMps212Config(&(pUsacCpeConfig->usacMps212Config), bsResidualCoding, hBitBuf);
  }

  return;
}

static void WriteExtElementConfig(USAC_EXT_CONFIG *pUsacExtConfig, HANDLE_BIT_BUF hBitBuf) {
  int length;
  if (pUsacExtConfig != NULL) {
    writeEscapedValue(hBitBuf, pUsacExtConfig->usacExtElementType, 4, 8, 16);
    writeEscapedValue(hBitBuf, pUsacExtConfig->usacExtElementConfigLength, 4, 8, 16);

    WriteBits(hBitBuf, pUsacExtConfig->usacExtElementDefaultLengthPresent, 1);
    if (pUsacExtConfig->usacExtElementDefaultLengthPresent) {
      writeEscapedValue(hBitBuf, pUsacExtConfig->usacExtElementDefaultLength - 1, 8, 16, 0);
    }

    WriteBits(hBitBuf, pUsacExtConfig->usacExtElementPayloadFrag, 1);
    switch (pUsacExtConfig->usacExtElementType) {
      case USAC_ID_EXT_ELE_FILL:
        break;
      case USAC_ID_EXT_ELE_MPEGS:

        assert(0);
        break;
      case USAC_ID_EXT_ELE_SAOC:

        assert(0);
        break;
      case USAC_ID_EXT_ELE_PREROLL:

        assert(pUsacExtConfig->usacExtElementConfigLength == 0);
        break;
      case USAC_ID_EXT_ELE_UNI_DRC:

        length = pUsacExtConfig->usacExtElementConfigLength;
        while (length > 0) {
          length--;
          WriteBits(hBitBuf, pUsacExtConfig->usacExtElementConfigPayload[pUsacExtConfig->usacExtElementConfigLength - 1 - length], 8);
        }
        break;
      default:
        length = pUsacExtConfig->usacExtElementConfigLength;
        while (length > 0) {
          length--;
          WriteBits(hBitBuf, pUsacExtConfig->usacExtElementConfigPayload[pUsacExtConfig->usacExtElementConfigLength - 1 - length], 8);
        }
        break;
    }
  }
  return;
}
static void WriteUsacConfigExtension(USAC_CONFIG_EXTENSION *pUsacConfigExtension, HANDLE_BIT_BUF hBitBuf) {
  if (pUsacConfigExtension != NULL) {
    int usacConfigExtensionPresent = (pUsacConfigExtension->numConfigExtensions > 0);
    WriteBits(hBitBuf, usacConfigExtensionPresent, 1);

    if (pUsacConfigExtension->numConfigExtensions > 0) {
      unsigned int confExtIdx = 0;

      writeEscapedValue(hBitBuf, pUsacConfigExtension->numConfigExtensions - 1, 2, 4, 8);

      for (confExtIdx = 0; confExtIdx < pUsacConfigExtension->numConfigExtensions; confExtIdx++) {
        unsigned int byteCnt = 0;
        unsigned int confExtLength = pUsacConfigExtension->usacConfigExtLength[confExtIdx];

        writeEscapedValue(hBitBuf, pUsacConfigExtension->usacConfigExtType[confExtIdx], 4, 8, 16);
        writeEscapedValue(hBitBuf, pUsacConfigExtension->usacConfigExtLength[confExtIdx], 4, 8, 16);

        switch (pUsacConfigExtension->usacConfigExtType[confExtIdx]) {
          case USAC_ID_CONFIG_EXT_FILL:
            while (confExtLength > 0) {
              confExtLength--;
              WriteBits(hBitBuf, 0xa5, 8);
              byteCnt++;
            }
            break;
          case USAC_ID_CONFIG_EXT_LOUDNESS_INFO:
            while (confExtLength > 0) {
              confExtLength--;
              WriteBits(hBitBuf, pUsacConfigExtension->usacConfigExtPayload[confExtIdx][byteCnt++], 8);
            }
            break;
          case USAC_ID_CONFIG_EXT_STREAM_ID:
            while (confExtLength > 0) {
              confExtLength--;
              WriteBits(hBitBuf, pUsacConfigExtension->usacConfigExtPayload[confExtIdx][byteCnt++], 8);
            }
            break;
          default:
            while (confExtLength > 0) {
              confExtLength--;
              WriteBits(hBitBuf, pUsacConfigExtension->usacConfigExtPayload[confExtIdx][byteCnt++], 8);
            }
            break;
        }
        assert(pUsacConfigExtension->usacConfigExtLength[confExtIdx] == byteCnt);
      }
    }
  }
  return;
}

static void WriteUsacDecoderConfig(USAC_DECODER_CONFIG *pUsacDecoderConfig, USAC_CONFIG_SBR_RATIO_INDEX sbrRatioIndex, HANDLE_BIT_BUF hBitBuf) {
  int elemIdx = 0;

  writeEscapedValue(hBitBuf, pUsacDecoderConfig->numElements - 1, 4, 8, 16);

  for (elemIdx = 0; elemIdx < (int)pUsacDecoderConfig->numElements; elemIdx++) {
    WriteBits(hBitBuf, pUsacDecoderConfig->usacElementType[elemIdx], 2);

    switch (pUsacDecoderConfig->usacElementType[elemIdx]) {
      case USAC_ELEMENT_TYPE_SCE:
        WriteSceConfig(&(pUsacDecoderConfig->usacElementConfig[elemIdx].usacSceConfig), sbrRatioIndex, hBitBuf);
        break;
      case USAC_ELEMENT_TYPE_CPE:
        WriteCpeConfig(&(pUsacDecoderConfig->usacElementConfig[elemIdx].usacCpeConfig), sbrRatioIndex, hBitBuf);
        break;
      case USAC_ELEMENT_TYPE_LFE:

        break;
      case USAC_ELEMENT_TYPE_EXT:
        WriteExtElementConfig(&(pUsacDecoderConfig->usacElementConfig[elemIdx].usacExtConfig), hBitBuf);
        break;
      default:
        assert(0);
        break;
    }
  }

  return;
}

static void WriteUsacChannelConfig(USAC_CHANNEL_CONFIG *pUsacChannelConfig, HANDLE_BIT_BUF hBitBuf) {
  unsigned int i = 0;

  writeEscapedValue(hBitBuf, pUsacChannelConfig->numOutChannels, 5, 8, 16);

  for (i = 0; i < pUsacChannelConfig->numOutChannels; i++) {
    WriteBits(hBitBuf, pUsacChannelConfig->usacOutputChannelPos[i], 5);
  }
}

void WriteUsacConfig(HANDLE_ASC hAsc,
                     struct AOT_SPECIFIC_CONFIG *aotSpecificConfig,
                     HANDLE_BIT_BUF hBitBuf) {
  HANDLE_USACC hUsacc = (HANDLE_USACC)aotSpecificConfig;
  unsigned int usacSamplingFrequencyIndex;
  int coreSbrFrameLengthIndex;

  usacSamplingFrequencyIndex = getUsacSamplingFrequencyIndex(hUsacc->usacSamplingFrequency);
  WriteBits(hBitBuf, usacSamplingFrequencyIndex, 5);
  if (usacSamplingFrequencyIndex == 0x1f) {
    WriteBits(hBitBuf, hUsacc->usacSamplingFrequency, 24);
  }

  coreSbrFrameLengthIndex = getCoreSbrFrameLengthIndex(hUsacc->outputFrameLength, hUsacc->sbrRatioIndex);
  WriteBits(hBitBuf, coreSbrFrameLengthIndex, 3);

  WriteBits(hBitBuf, hUsacc->channelConfigurationIndex, 5);
  if (hUsacc->channelConfigurationIndex == 0) {
    WriteUsacChannelConfig(&hUsacc->usacChannelConfig, hBitBuf);
  }

  WriteUsacDecoderConfig(&hUsacc->usacDecoderConfig, hUsacc->sbrRatioIndex, hBitBuf);

  WriteUsacConfigExtension(&hUsacc->usacConfigExtension, hBitBuf);

  (void)hAsc;

  return;
}
