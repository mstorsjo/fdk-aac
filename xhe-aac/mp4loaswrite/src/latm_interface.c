
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

#include <string.h>
#include <math.h>
#include <limits.h>

#include "latm_interface.h"
#include "bit_buf.h"
#include "mp4audioheader.h"

static const int celpFrameLengthTable[64] = {
    154, 170, 186, 147, 156, 165, 114, 120,
    186, 126, 132, 138, 142, 146, 154, 166,
    174, 182, 190, 198, 206, 210, 214, 110,
    114, 118, 120, 122, 218, 230, 242, 254,
    266, 278, 286, 294, 318, 342, 358, 374,
    390, 406, 422, 136, 142, 148, 154, 160,
    166, 170, 174, 186, 198, 206, 214, 222,
    230, 238, 216, 160, 280, 338, 0, 0};

static unsigned int CountLatmFixBitDemandHeader(HANDLE_LATM_STREAM_INFO hAss);

static unsigned int CountLatmVarBitDemandHeader(HANDLE_LATM_STREAM_INFO hAss, unsigned int streamDataLength);

unsigned int
CountLatmBitDemandHeader(HANDLE_LATM_STREAM_INFO hAss, unsigned int streamDataLength, int nMode) {
  unsigned int bitDemand = 0;
  unsigned int sanity;

  switch (nMode) {
    case 0:
      bitDemand = CountLatmFixBitDemandHeader(hAss);
      break;
    case 1:
      bitDemand = CountLatmVarBitDemandHeader(hAss, streamDataLength);
      break;
    case 2:
      bitDemand = CountLatmFixBitDemandHeader(hAss);
      sanity = streamDataLength > bitDemand;
      bitDemand += CountLatmVarBitDemandHeader(hAss, (sanity) ? (streamDataLength - bitDemand) : bitDemand);
      break;
  }

  return bitDemand;
}

static unsigned int CountLatmFixBitDemandHeader(HANDLE_LATM_STREAM_INFO hAss) {
  unsigned int i;
  unsigned int bitDemand = 0;
  int insertSetupData = 0;

  if (hAss->subFrameCnt == 0) {
    bitDemand += 11;
    bitDemand += 13;
  }

  if (hAss->lsli.setupDataOutOfBandFlag == 0) {
    if (hAss->lsli.setupDataDistanceFrames > 0) {
      insertSetupData = (hAss->subFrameCnt == 0) && (hAss->lsli.syncFrameCounter == 0);
    } else if (hAss->lsli.setupDataDistanceFrames == -1) {
      insertSetupData = (hAss->subFrameCnt == 0) && (hAss->lsli.syncFrameCounter == 0);
    }

    bitDemand += 1;

    if (insertSetupData && !hAss->smcOutOfBitres) {
      bitDemand += hAss->streamMuxConfigBits;
    }
  }

  for (i = 0; i < hAss->otherDataLenBytes; i++) {
    bitDemand += 8;
  }

  if (bitDemand % 8) {
    unsigned int fillBits = 8 - (bitDemand % 8);
    bitDemand += fillBits;
  }

  return bitDemand;
}

static unsigned int CountLatmVarBitDemandHeader(HANDLE_LATM_STREAM_INFO hAss, unsigned int streamDataLength) {
  unsigned int bitDemand = 0;
  int prog, layer;

  if (hAss->allStreamsSameTimeFraming) {
    for (prog = 0; prog < hAss->noProgram; prog++) {
      for (layer = 0; layer < MAX_LAYERS; layer++) {
        LATM_LAYER_INFO *p_linfo = &(hAss->m_linfo[prog][layer]);

        if (p_linfo->streamID >= 0) {
          switch (p_linfo->frameLengthType) {
            case 0:
              if (streamDataLength > 0) {
                streamDataLength -= bitDemand;
                while (streamDataLength >= 255 * 8) {
                  bitDemand += 8;
                  streamDataLength -= (255 * 8);
                }
                bitDemand += 8;
              }
              break;

            case 1:
            case 4:
            case 6:
              bitDemand += 2;
              break;

            default:
              return 0;
          }
        }
      }
    }
  } else {
    switch (hAss->varMode) {
      case LATMVAR_SIMPLE_SEQUENCE: {
        int streamCntPosition = SetWritePointer(hAss->hAssembleBuffer, 0);
        bitDemand += 4;

        hAss->varStreamCnt = 0;
        for (prog = 0; prog < hAss->noProgram; prog++) {
          for (layer = 0; layer < MAX_LAYERS; layer++) {
            LATM_LAYER_INFO *p_linfo = &(hAss->m_linfo[prog][layer]);

            if (p_linfo->streamID >= 0) {
              bitDemand += 4;
              switch (p_linfo->frameLengthType) {
                case 0:
                  streamDataLength -= bitDemand;
                  while (streamDataLength >= 255 * 8) {
                    bitDemand += 8;
                    streamDataLength -= (255 * 8);
                  }

                  bitDemand += 8;
                  bitDemand += 1;
                  break;

                case 1:
                case 4:
                case 6:

                  break;

                default:
                  return 0;
              }
              hAss->varStreamCnt++;
            }
          }
        }
        bitDemand += 4;
        UpdateBitstreamField(hAss->hAssembleBuffer, streamCntPosition, hAss->varStreamCnt - 1, 4);
      } break;

      default:
        return 0;
    }
  }

  return bitDemand;
}

static HANDLE_ERROR_INFO CreateStreamMuxConfig(HANDLE_LATM_STREAM_INFO hAss,
                                               HANDLE_CODER_CONFIG_INFO layerConfig[LATM_MAX_PROGRAMS][MAX_LAYERS]) {
  HANDLE_ERROR_INFO err = noError;

  int streamIDcnt, layer, prog, tmp;

  unsigned short coreFrameOffset = 0;
  unsigned short audioMuxVersion = 0;

  hAss->streamMuxConfigBits = 0;

  WriteBits(hAss->hSetupData, audioMuxVersion, 1);

  if (audioMuxVersion == 0) {
    WriteBits(hAss->hSetupData, hAss->allStreamsSameTimeFraming ? 1 : 0, 1);

    hAss->nrOfSubframesPosition = SetWritePointer(hAss->hSetupData, 0);
    WriteBits(hAss->hSetupData, hAss->nrOfSubframes - 1, 6);
    WriteBits(hAss->hSetupData, hAss->noProgram - 1, 4);

    hAss->streamMuxConfigBits += 11;

    streamIDcnt = 0;
    for (prog = 0; prog < hAss->noProgram; prog++) {
      int transLayer = 0;

      WriteBits(hAss->hSetupData, hAss->noLayer[prog] - 1, 3);
      hAss->streamMuxConfigBits += 3;

      for (layer = 0; layer < MAX_LAYERS; layer++) {
        LATM_LAYER_INFO *p_linfo = &(hAss->m_linfo[prog][layer]);
        CODER_CONFIG_INFO *p_lci = layerConfig[prog][layer];

        p_linfo->bufferFullnessPosition = -1;
        p_linfo->streamID = -1;

        if (p_lci->ascFlag != 0) {
          int useSameConfig = 0;

          if (transLayer > 0) {
            WriteBits(hAss->hSetupData, useSameConfig ? 1 : 0, 1);
            hAss->streamMuxConfigBits += 1;
          }
          if ((useSameConfig == 0) || (transLayer == 0)) {
            int ascSize = SetWritePointer(hAss->hSetupData, 0);

            if (p_lci->asc.channelConfiguration > 0) {
              if (p_lci->asc.channelConfiguration == 7 || p_lci->asc.channelConfiguration == 12 || p_lci->asc.channelConfiguration == 14) {
                p_linfo->m_nConsideredChannels = 7;
              } else if (p_lci->asc.channelConfiguration == 13) {
                p_linfo->m_nConsideredChannels = 22;
              } else if (p_lci->asc.channelConfiguration == 11) {
                p_linfo->m_nConsideredChannels = 6;
              } else if (p_lci->asc.channelConfiguration == 6) {
                p_linfo->m_nConsideredChannels = 5;
              } else if (p_lci->asc.channelConfiguration < 6) {
                p_linfo->m_nConsideredChannels = p_lci->asc.channelConfiguration;
              } else {
                return iisUtil_ERROR(CDI, "non Standard channel mapping not supported");
              }
            } else {
              return iisUtil_ERROR(CDI, "channel mapping below 0 not supported");
            }
            err = WriteAudioSpecificConfig(&p_lci->asc, hAss->hSetupData);
            if (err != noError) return err;
            ascSize = SetWritePointer(hAss->hSetupData, 0) - ascSize;
            hAss->streamMuxConfigBits += ascSize;
          }
          transLayer++;

          if (!hAss->allStreamsSameTimeFraming) {
            if (streamIDcnt >= LATM_MAX_STREAM_ID) return iisUtil_ERROR(CDI, " LATM stream ID exceeds capacity");
          }
          p_linfo->streamID = streamIDcnt++;

          switch (p_lci->asc.aot) {
            case AOT_AAC_MAIN:
            case AOT_AAC_LC:
            case AOT_AAC_SSR:
            case AOT_AAC_LTP:
            case AOT_AAC_SCAL:
            case AOT_ER_AAC_LC:
            case AOT_ER_AAC_LTP:
            case AOT_ER_AAC_SCAL:
            case AOT_ER_AAC_LD:
            case AOT_ER_AAC_ELD:
            case AOT_USAC:
            case AOT_MP2_AAC_MAIN:
            case AOT_MP2_AAC_LC:
            case AOT_MP2_AAC_SSR:
              p_linfo->frameLengthType = 0;

              WriteBits(hAss->hSetupData, p_linfo->frameLengthType, 3);

              p_linfo->bufferFullnessPosition = GetBitsAvail(hAss->hSetupData);
              WriteBits(hAss->hSetupData, 0xff, 8);

              hAss->streamMuxConfigBits += 11;
              if (!hAss->allStreamsSameTimeFraming) {
                CODER_CONFIG_INFO *p_lci_prev = layerConfig[prog][layer - 1];
                if (((p_lci->asc.aot == AOT_AAC_SCAL) || (p_lci->asc.aot == AOT_ER_AAC_SCAL)) &&
                    ((p_lci_prev->asc.aot == AOT_CELP) || (p_lci_prev->asc.aot == AOT_ER_CELP))) {
                  WriteBits(hAss->hSetupData, coreFrameOffset, 6);
                  hAss->streamMuxConfigBits += 6;
                }
              }
              break;

            case AOT_TWIN_VQ:
              p_linfo->frameLengthType = 1;
              tmp = ((p_lci->bitsFrame + 7) >> 3) - 20;
              if ((tmp < 0)) {
                return iisUtil_ERROR(CDI, "Illegal TwinVQ frame length");
              }
              WriteBits(hAss->hSetupData, p_linfo->frameLengthType, 3);
              WriteBits(hAss->hSetupData, tmp, 9);
              hAss->streamMuxConfigBits += 12;

              p_linfo->frameLengthBits = (tmp + 20) << 3;
              break;

            case AOT_CELP:
              p_linfo->frameLengthType = 4;
              WriteBits(hAss->hSetupData, p_linfo->frameLengthType, 3);
              hAss->streamMuxConfigBits += 3;
              {
                int i;
                for (i = 0; i < 62; i++) {
                  if (celpFrameLengthTable[i] == p_lci->bitsFrame)
                    break;
                }
                if (i >= 62) {
                  return iisUtil_ERROR(CDI, "Illegal CELP Bitrate in LATM setup");
                }

                WriteBits(hAss->hSetupData, i, 6);
                hAss->streamMuxConfigBits += 6;
              }
              p_linfo->frameLengthBits = p_lci->bitsFrame;
              break;

            case AOT_HVXC:
              p_linfo->frameLengthType = 6;
              WriteBits(hAss->hSetupData, p_linfo->frameLengthType, 3);
              hAss->streamMuxConfigBits += 3;
              {
                int i;

                if (p_lci->bitsFrame == 40) {
                  i = 0;
                } else if (p_lci->bitsFrame == 80) {
                  i = 1;
                } else {
                  return iisUtil_ERROR(CDI, "Illegal HVXC Bitrate in LATM setup");
                }
                WriteBits(hAss->hSetupData, i, 1);
                hAss->streamMuxConfigBits += 1;
              }
              p_linfo->frameLengthBits = p_lci->bitsFrame;
              break;

            case AOT_NULL_OBJECT:
            default:
              return iisUtil_ERROR(CDI, "Illegal Object type during creation of LATM setup data");
          }
        }
      }
    }

    WriteBits(hAss->hSetupData, (hAss->otherDataLenBytes > 0) ? 1 : 0, 1);
    hAss->streamMuxConfigBits += 1;

    if (hAss->otherDataLenBytes > 0) {
      int otherDataLenTmp = hAss->otherDataLenBytes;
      int escCnt = 0;
      int otherDataLenEsc = 1;

      while (otherDataLenTmp) {
        otherDataLenTmp >>= 8;
        escCnt++;
      }

      do {
        otherDataLenTmp = (hAss->otherDataLenBytes >> (escCnt * 8)) & 0xFF;
        escCnt--;
        otherDataLenEsc = escCnt > 0;

        WriteBits(hAss->hSetupData, otherDataLenEsc, 1);
        WriteBits(hAss->hSetupData, otherDataLenTmp, 8);
        hAss->streamMuxConfigBits += 9;
      } while (otherDataLenEsc);
    }

    {
      unsigned short crcCheckPresent = 0;
      unsigned short crcCheckSum = 0;

      WriteBits(hAss->hSetupData, crcCheckPresent, 1);
      hAss->streamMuxConfigBits += 1;
      if (crcCheckPresent) {
        WriteBits(hAss->hSetupData, crcCheckSum, 8);
        hAss->streamMuxConfigBits += 8;
      }
    }

  } else {
  }

  return err;
}

static HANDLE_ERROR_INFO UpdateStreamMuxConfig(HANDLE_LATM_STREAM_INFO hAss, int bufferFullness[LATM_MAX_PROGRAMS][MAX_LAYERS]) {
  HANDLE_ERROR_INFO err = noError;

  int prog, layer;

  for (prog = 0; prog < hAss->noProgram; prog++) {
    for (layer = 0; layer < MAX_LAYERS; layer++) {
      LATM_LAYER_INFO *p_linfo = &(hAss->m_linfo[prog][layer]);

      if (bufferFullness) {
        if (p_linfo->bufferFullnessPosition >= 0) {
          {
            int latmBufferFullness = 0xff;
            if (p_linfo->m_nConsideredChannels > 0 && (bufferFullness[prog][layer] >= 0)) {
              latmBufferFullness = (bufferFullness[prog][layer] / p_linfo->m_nConsideredChannels) / 32;
            }
            UpdateBitstreamField(hAss->hSetupData, p_linfo->bufferFullnessPosition, latmBufferFullness, 8);
          }
        }
      }

      if (hAss->nrOfSubframesPosition >= 0) {
        UpdateBitstreamField(hAss->hSetupData, hAss->nrOfSubframesPosition, hAss->nrOfSubframes - 1, 6);
      }
    }
  }

  {
    int setupDataLength = GetBitsAvail(hAss->hSetupData);

    ngsCopyBits(hAss->hAssembleBuffer, hAss->hSetupData, setupDataLength);
    SetReadPointer(hAss->hSetupData, -setupDataLength);
  }

  return err;
}

static HANDLE_ERROR_INFO WriteAuPayloadLengthInfo(HANDLE_BIT_BUF hBitStream, int AuLengthBits) {
  int restBytes;

  while (AuLengthBits >= 255 * 8) {
    WriteBits(hBitStream, 255, 8);
    AuLengthBits -= (255 * 8);
  }

  restBytes = (AuLengthBits + 7) >> 3;
  WriteBits(hBitStream, restBytes, 8);

  return noError;
}

static HANDLE_ERROR_INFO WriteAu(HANDLE_BIT_BUF hBitStream, HANDLE_BIT_BUF hLayerStream, int AuLengthBits) {
  int restBytes, fillBits;

  while (AuLengthBits > 255 * 8) {
    ngsCopyBits(hBitStream, hLayerStream, 255 * 8);
    AuLengthBits -= 255 * 8;
  }

  restBytes = (AuLengthBits + 7) >> 3;
  ngsCopyBits(hBitStream, hLayerStream, AuLengthBits);

  fillBits = (restBytes << 3) - AuLengthBits;
  WriteBits(hBitStream, 0, fillBits);

  return noError;
}

static HANDLE_ERROR_INFO CreatePayloadLengthInfo(HANDLE_LATM_STREAM_INFO hAss, HANDLE_BIT_BUF hLayerBitstream[LATM_MAX_PROGRAMS][MAX_LAYERS]) {
  HANDLE_ERROR_INFO err = noError;

  int prog, layer, tmp;

  if (hAss->allStreamsSameTimeFraming) {
    for (prog = 0; prog < hAss->noProgram; prog++) {
      for (layer = 0; layer < MAX_LAYERS; layer++) {
        LATM_LAYER_INFO *p_linfo = &(hAss->m_linfo[prog][layer]);

        if (p_linfo->streamID >= 0) {
          switch (p_linfo->frameLengthType) {
            case 0:
              err = DecAuReady(hLayerBitstream[prog][layer], &p_linfo->frameLengthBits);
              if (err) return err;

              err = WriteAuPayloadLengthInfo(hAss->hAssembleBuffer, p_linfo->frameLengthBits);
              if (err) return err;
              break;

            case 1:
            case 4:
            case 6:
              DecAuReady(hLayerBitstream[prog][layer], &tmp);
              assert(tmp == p_linfo->frameLengthBits);
              break;

            default:
              return iisUtil_ERROR(CDI, "LATM frame length type not yet supported");
          }
        }
      }
    }
  } else {
    switch (hAss->varMode) {
      case LATMVAR_SIMPLE_SEQUENCE: {
        int streamCntPosition = SetWritePointer(hAss->hAssembleBuffer, 0);
        WriteBits(hAss->hAssembleBuffer, 0, 4);

        hAss->varStreamCnt = 0;
        for (prog = 0; prog < hAss->noProgram; prog++) {
          for (layer = 0; layer < MAX_LAYERS; layer++) {
            LATM_LAYER_INFO *p_linfo = &(hAss->m_linfo[prog][layer]);

            if (p_linfo->streamID >= 0) {
              while (GetAuReady(hLayerBitstream[prog][layer])) {
                VAR_MUX_SETUP_INFO *p_vmsi = &(hAss->vmsi[hAss->varStreamCnt]);

                WriteBits(hAss->hAssembleBuffer, p_linfo->streamID, 4);

                p_vmsi->chunkLenBits = p_linfo->frameLengthBits;
                p_vmsi->prog = prog;
                p_vmsi->layer = layer;

                switch (p_linfo->frameLengthType) {
                  case 0:
                    DecAuReady(hLayerBitstream[prog][layer], &p_vmsi->chunkLenBits);
                    err = WriteAuPayloadLengthInfo(hAss->hAssembleBuffer, p_vmsi->chunkLenBits);
                    if (err) return err;

                    WriteBits(hAss->hAssembleBuffer, 1, 1);

                    break;

                  case 1:
                  case 4:
                  case 6:
                    DecAuReady(hLayerBitstream[prog][layer], &tmp);
                    assert(tmp == p_linfo->frameLengthBits);

                    break;

                  default:
                    return iisUtil_ERROR(CDI, "LATM frame length type not yet supported");
                }

                hAss->varStreamCnt++;
              }
            }
          }
        }

        UpdateBitstreamField(hAss->hAssembleBuffer, streamCntPosition, hAss->varStreamCnt - 1, 4);
      } break;

      default:
        return iisUtil_ERROR(CDI, "LATM unknown variable stream mode");
    }
  }

  return err;
}

static HANDLE_ERROR_INFO CreatePayload(HANDLE_LATM_STREAM_INFO hAss, HANDLE_BIT_BUF hLayerBitstream[LATM_MAX_PROGRAMS][MAX_LAYERS]) {
  HANDLE_ERROR_INFO err = noError;

  int prog, layer;

  if (hAss->allStreamsSameTimeFraming) {
    for (prog = 0; prog < hAss->noProgram; prog++) {
      for (layer = 0; layer < MAX_LAYERS; layer++) {
        LATM_LAYER_INFO *p_linfo = &(hAss->m_linfo[prog][layer]);

        if (p_linfo->streamID >= 0) {
          switch (p_linfo->frameLengthType) {
            case 0:
              WriteAu(hAss->hAssembleBuffer, hLayerBitstream[prog][layer], p_linfo->frameLengthBits);
              break;

            case 1:
            case 4:
            case 6:
              ngsCopyBits(hAss->hAssembleBuffer, hLayerBitstream[prog][layer], p_linfo->frameLengthBits);
              break;

            default:
              return iisUtil_ERROR(CDI, "LATM frame length type not yet supported");
          }
        }
      }
    }
  } else {
    int id;

    for (id = 0; id < hAss->varStreamCnt; id++) {
      VAR_MUX_SETUP_INFO *p_vmsi = &(hAss->vmsi[id]);

      ngsCopyBits(hAss->hAssembleBuffer, hLayerBitstream[p_vmsi->prog][p_vmsi->layer], p_vmsi->chunkLenBits);
    }
  }

  return err;
}

static HANDLE_ERROR_INFO AdvanceAudioMuxElement(
    HANDLE_LATM_STREAM_INFO hAss,
    int insertMuxSetup,
    HANDLE_BIT_BUF hLayerBitstream[LATM_MAX_PROGRAMS][MAX_LAYERS],
    int bufferFullness[LATM_MAX_PROGRAMS][MAX_LAYERS],
    LOASWRITER_SMC_WRITTEN *smcWritten) {
  HANDLE_ERROR_INFO err = noError;

  unsigned int i;

  if (smcWritten != NULL) {
    *smcWritten = NO_SMC_WRITTEN;
  }

  if (!hAss->lsli.setupDataOutOfBandFlag) {
    if (insertMuxSetup) {
      WriteBits(hAss->hAssembleBuffer, 0, 1);
      if (smcWritten != NULL) {
        *smcWritten = SMC_WRITTEN;
      }

      err = UpdateStreamMuxConfig(hAss, bufferFullness);
      if (err) return err;
    } else {
      WriteBits(hAss->hAssembleBuffer, 1, 1);
    }
  }

  for (i = 0; i < (unsigned int)hAss->nrOfSubframes; i++) {
    err = CreatePayloadLengthInfo(hAss, hLayerBitstream);
    if (err) return err;

    err = CreatePayload(hAss, hLayerBitstream);
    if (err) return err;
  }

  for (i = 0; i < hAss->otherDataLenBytes; i++) {
    WriteBits(hAss->hAssembleBuffer, 0, 8);
  }

  return err;
}

static int allStreamsSameTimeFraming(int noProgram, HANDLE_CODER_CONFIG_INFO layerConfig[LATM_MAX_PROGRAMS][MAX_LAYERS], int noLayer[]) {
  int prog, layer;

  int lastNoSamples = -1;
  int minFrameSamples = INT_MAX;
  int maxFrameSamples = 0;

  int highestSamplingRate = -1;

  for (prog = 0; prog < noProgram; prog++) {
    noLayer[prog] = 0;

    for (layer = MAX_LAYERS - 1; layer >= 0; layer--) {
      if (layerConfig[prog][layer]->ascFlag != 0) {
        int hsfSamplesFrame;

        noLayer[prog]++;

        if (highestSamplingRate < 0)
          highestSamplingRate = layerConfig[prog][layer]->samplingRate;

        hsfSamplesFrame = layerConfig[prog][layer]->samplesFrame * highestSamplingRate / layerConfig[prog][layer]->samplingRate;

        if (hsfSamplesFrame <= minFrameSamples) minFrameSamples = hsfSamplesFrame;
        if (hsfSamplesFrame >= maxFrameSamples) maxFrameSamples = hsfSamplesFrame;

        if (lastNoSamples == -1) {
          lastNoSamples = hsfSamplesFrame;
        } else {
          if (hsfSamplesFrame != lastNoSamples) {
            return 0;
          }
        }
      }
    }
  }

  return 1;
}

HANDLE_ERROR_INFO
CreateLatmStream(
    HANDLE_LATM_STREAM_INFO *p_hAss,
    LOASWRITER_MODE mode,
    int fractDelayPresent,
    int noProgram,
    HANDLE_CODER_CONFIG_INFO layerConfig[LATM_MAX_PROGRAMS][MAX_LAYERS]) {
  HANDLE_ERROR_INFO err = noError;

  *p_hAss = (LATM_STREAM_INFO *)iisMalloc(sizeof(LATM_STREAM_INFO));

  if (mode == MODE_LOAS || mode == MODE_LOAS_NO_SMC) {
    (*p_hAss)->mode = MODE_LOAS;
  } else {
    (*p_hAss)->mode = MODE_LATM;
  }

  if (mode == MODE_LATM_NO_SMC || mode == MODE_LOAS_NO_SMC) {
    (*p_hAss)->lsli.setupDataOutOfBandFlag = 1;
  } else {
    (*p_hAss)->lsli.setupDataOutOfBandFlag = 0;
  }

  (*p_hAss)->noProgram = noProgram;
  (*p_hAss)->allStreamsSameTimeFraming = allStreamsSameTimeFraming(noProgram, layerConfig, (*p_hAss)->noLayer);

  (*p_hAss)->fractDelayPresent = fractDelayPresent;
  (*p_hAss)->otherDataLenBytes = 0;

  (*p_hAss)->varMode = LATMVAR_SIMPLE_SEQUENCE;

  (*p_hAss)->hSetupData = CreateBitBuffer(200 * 8);

  (*p_hAss)->hAssembleBuffer = CreateBitBuffer((8192 + 16) * 8);

  (*p_hAss)->subFrameCnt = 0;
  (*p_hAss)->nrOfSubframes = NROFSUBFRAMES_DEFAULT;
  (*p_hAss)->nrOfSubframes_next = NROFSUBFRAMES_DEFAULT;
  (*p_hAss)->nrOfSubframesPosition = -1;

  (*p_hAss)->lsli.audioMuxLengthBytes = 0;
  (*p_hAss)->lsli.syncFrameCounter = 0;
  (*p_hAss)->lsli.setupDataDistanceFrames = SMC_INTERVALL_DEFAULT;

  (*p_hAss)->smcOutOfBitres = 0;

  err = CreateStreamMuxConfig(*p_hAss, layerConfig);

  return err;
}

HANDLE_ERROR_INFO
SetSmcInterval(HANDLE_LATM_STREAM_INFO hAss,
               int setupDataDistanceFrames) {
  HANDLE_ERROR_INFO error = noError;

  if (setupDataDistanceFrames < -1) {
    return iisUtil_ERROR(CDI, "invalid SMC distance");
  }

  hAss->lsli.setupDataDistanceFrames = setupDataDistanceFrames;

  return error;
}

HANDLE_ERROR_INFO
SetSmcOutOfBitres(HANDLE_LATM_STREAM_INFO hAss,
                  int smcOutOfBitres) {
  HANDLE_ERROR_INFO error = noError;

  if (!hAss) {
    return iisUtil_ERROR(CDI, "invalid SMC handle");
  }

  if (smcOutOfBitres < 0 || smcOutOfBitres > 1) {
    return iisUtil_ERROR(CDI, "invalid SMC out of bitres mode");
  }

  hAss->smcOutOfBitres = smcOutOfBitres;
  return error;
}

HANDLE_ERROR_INFO
SetNrOfSubframes(HANDLE_LATM_STREAM_INFO hAss,
                 int nrOfSubframes_next) {
  HANDLE_ERROR_INFO error = noError;

  if (nrOfSubframes_next < 1) {
    return iisUtil_ERROR(CDI, "invalid nrOfSubframes");
  }

  hAss->nrOfSubframes_next = nrOfSubframes_next;

  if ((hAss->subFrameCnt == 0) && (hAss->lsli.syncFrameCounter == 0)) {
    hAss->nrOfSubframes = nrOfSubframes_next;
  }

  return error;
}

HANDLE_ERROR_INFO
AdvanceLatmStream(HANDLE_LATM_STREAM_INFO hAss,
                  HANDLE_BIT_BUF hLayerBitstream[LATM_MAX_PROGRAMS][MAX_LAYERS],
                  int bufferFullness[LATM_MAX_PROGRAMS][MAX_LAYERS],
                  unsigned char *outBuffer,
                  int *outNoBytes,
                  LOASWRITER_FRAME_INFO_HANDLE hFrameInfo) {
  HANDLE_ERROR_INFO err = noError;

  int audioMuxPos = 0, actPos = 0;
  int ameLength = 0;
  int i;
  int prog, layer;
  unsigned int auReady = 0;

  for (prog = 0; prog < hAss->noProgram; prog++) {
    for (layer = 0; layer < hAss->noLayer[prog]; layer++) {
      if (hLayerBitstream[prog][layer]) {
        if (GetAuReady(hLayerBitstream[prog][layer]) >= hAss->nrOfSubframes)
          auReady = 1;
      }
    }
  }
  if (auReady == 0) {
    *outNoBytes = 0;
    hAss->subFrameCnt++;
    if (hAss->subFrameCnt >= hAss->nrOfSubframes) {
      return iisUtil_ERROR(CDI, "error in subframe count");
    }

    return noError;
  }

  if (hAss->mode == MODE_LOAS) {
    WriteBits(hAss->hAssembleBuffer, 0x2B7, 11);
    hAss->lsli.audioMuxLengthBytes = 0;
    audioMuxPos = SetWritePointer(hAss->hAssembleBuffer, 0);
    WriteBits(hAss->hAssembleBuffer, hAss->lsli.audioMuxLengthBytes, 13);
  }

  ameLength = SetWritePointer(hAss->hAssembleBuffer, 0);

  {
    int fillBits, tmp;
    int insertSetupData = 0;
    LOASWRITER_SMC_WRITTEN smcWritten = NO_SMC_WRITTEN;

    if (hAss->lsli.setupDataDistanceFrames > 0) {
      insertSetupData = (hAss->lsli.syncFrameCounter == 0);
    } else if (hAss->lsli.setupDataDistanceFrames == -1) {
      insertSetupData = (hAss->lsli.syncFrameCounter == 0);
    }

    err = AdvanceAudioMuxElement(hAss, insertSetupData, hLayerBitstream, bufferFullness, &smcWritten);
    if (err) return err;

    if (hFrameInfo != NULL) {
      hFrameInfo->smcWritten = smcWritten;
    }

    tmp = GetBitsAvail(hAss->hAssembleBuffer);
    if (tmp % 8) {
      fillBits = 8 - (tmp % 8);
      WriteBits(hAss->hAssembleBuffer, 0, fillBits);
    }

    tmp = GetBitsAvail(hAss->hAssembleBuffer);
    if (tmp % 8) return iisUtil_ERROR(CDI, "LATM byte alignment error");

    *outNoBytes = tmp >> 3;
  }

  if (hAss->mode == MODE_LOAS) {
    actPos = SetWritePointer(hAss->hAssembleBuffer, 0);
    if (actPos < ameLength)
      actPos += GetBitBufSize(hAss->hAssembleBuffer);
    ameLength = actPos - ameLength;

    if (actPos < audioMuxPos)
      actPos += GetBitBufSize(hAss->hAssembleBuffer);
    SetWritePointer(hAss->hAssembleBuffer, audioMuxPos - actPos);

    hAss->lsli.audioMuxLengthBytes = ameLength >> 3;
    WriteBits(hAss->hAssembleBuffer, hAss->lsli.audioMuxLengthBytes, 13);

    SetWritePointer(hAss->hAssembleBuffer, actPos - (audioMuxPos + 13));
  }

  for (i = 0; i < *outNoBytes; i++) {
    outBuffer[i] = (unsigned char)ReadBits(hAss->hAssembleBuffer, 8);
  }

  hAss->lsli.syncFrameCounter++;
  if ((hAss->lsli.syncFrameCounter >= hAss->lsli.setupDataDistanceFrames) && (hAss->lsli.setupDataDistanceFrames != -1)) {
    hAss->lsli.syncFrameCounter = 0;
    hAss->nrOfSubframes = hAss->nrOfSubframes_next;
  }

  hAss->subFrameCnt = 0;

  return noError;
}

HANDLE_ERROR_INFO
DeleteLatmStream(HANDLE_LATM_STREAM_INFO hAss) {
  if (hAss) {
    DeleteBitBuffer(hAss->hAssembleBuffer);
    DeleteBitBuffer(hAss->hSetupData);

    iisFree(hAss);
  }

  return noError;
}

HANDLE_ERROR_INFO
GetLatmStreamMuxConfig(HANDLE_LATM_STREAM_INFO hAss,
                       unsigned char *buffer,
                       int *nBits) {
  int nBytes = 0;
  int resBits = 0;
  int i = 0;

  if (hAss == NULL) {
    return iisUtil_ERROR(CDI, "invalid LATM stream info handle");
  }

  nBytes = hAss->streamMuxConfigBits >> 3;
  resBits = hAss->streamMuxConfigBits % 8;

  for (i = 0; i < nBytes; i++) {
    buffer[i] = (unsigned char)ReadBits(hAss->hSetupData, 8);
  }

  if (resBits > 0) {
    buffer[nBytes] = (unsigned char)ReadBits(hAss->hSetupData, resBits);
    buffer[nBytes] <<= (8 - resBits);
  }
  *nBits = hAss->streamMuxConfigBits;

  SetReadPointer(hAss->hSetupData, -*nBits);

  return noError;
}

HANDLE_ERROR_INFO
TriggerSmc(HANDLE_LATM_STREAM_INFO hAss) {
  HANDLE_ERROR_INFO error = noError;

  if (hAss == NULL) {
    return iisUtil_ERROR(CDI, "invalid Handle");
  }

  if (hAss->lsli.setupDataDistanceFrames == -1) {
    hAss->lsli.syncFrameCounter = 0;
  } else {
    return iisUtil_ERROR(CDI, "invalid Configuration (constant intervall is set, then no trigger is possible)");
  }

  return error;
}
