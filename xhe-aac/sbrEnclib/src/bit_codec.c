
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

#include <assert.h>

#include "iisutillib.h"
#include "sbr_main.h"
#include "cmondata.h"
#include "bit_buf.h"

#define SI_ID_BITS_AAC 3

#define SI_FILL_COUNT_BITS 4
#define SI_FILL_ESC_COUNT_BITS 8
#define SI_FILL_EXTENTION_BITS 4
#define ID_FIL 6

int CreateSbrBitInfo(HANDLE_SBR_BIT_DATA *pHandleSbrBit) {
  HANDLE_SBR_BIT_DATA hSbrBit;

  *pHandleSbrBit = NULL;

  hSbrBit = (HANDLE_SBR_BIT_DATA)iisCalloc(1, sizeof(SBR_BIT_DATA));

  if (hSbrBit == NULL) {
    return 1;
  }

  *pHandleSbrBit = hSbrBit;

  return 0;
}

void DeleteSbrBitInfo(HANDLE_SBR_BIT_DATA *pHandleSbrBit) {
  if (*pHandleSbrBit != NULL) {
    iisFree(*pHandleSbrBit);
    *pHandleSbrBit = NULL;
  }
}

int SbrHeaderNPayload(HANDLE_COMMON_DATA hCmonData) {
  return (hCmonData->sbrHdrBits + hCmonData->sbrDataBits);
}

int SbrCountBits(HANDLE_SBR_BIT_DATA hSbrBit,
                 HANDLE_COMMON_DATA hCmonData,
                 CODEC_TYPE coreCoder) {
  int sbrLoad = 0;
  int sigBits = 0;
  int filBits = 0;
  int sbrByteCnt = 0;
  int sbrExtByteCnt = 0;
  int maxByteCnt = 0;

  if (hCmonData == NULL)
    return 0;

  sbrLoad = hCmonData->sbrHdrBits + hCmonData->sbrDataBits;

  switch (coreCoder) {
    case CODEC_SAAC:
      maxByteCnt = (1 << SI_FILL_COUNT_BITS) - 1;

      sigBits += SI_ID_BITS_AAC + SI_FILL_COUNT_BITS;
      sbrLoad += SI_FILL_EXTENTION_BITS;

      if (hCmonData->sbrCrcLen) {
        sbrLoad += SI_SBR_CRC_BITS;
      }

      hSbrBit->sbrPayload = sbrLoad;

      filBits = (8 - (sbrLoad) % 8) % 8;
      sbrLoad += filBits;

      sbrByteCnt = sbrLoad >> 3;

      if (sbrByteCnt >= maxByteCnt) {
        sigBits += SI_FILL_ESC_COUNT_BITS;
        sbrExtByteCnt = sbrByteCnt - maxByteCnt + 1;
        sbrByteCnt = maxByteCnt;
      }
      break;

    default:

      assert(0);
      return 0;
  }

  sbrLoad += sigBits;

  hSbrBit->totalBits = sbrLoad;
  hSbrBit->fillBits = filBits;

  hSbrBit->sbrByteCnt = sbrByteCnt;
  hSbrBit->cntEscValue = maxByteCnt;
  hSbrBit->sbrExtByteCnt = sbrExtByteCnt;

  return sbrLoad;
}

void AppendSbrBitstream(HANDLE_SBR_BIT_DATA hSbrBit,
                        HANDLE_BIT_BUF hBitStream,
                        HANDLE_COMMON_DATA hCmonData,
                        CODEC_TYPE coreCoder) {
  const int nCrcBits = 10;
  int crcBitsStartPosition = 0, crcCalcStartPosition = 0;
  int crcBits;

  if (hCmonData == NULL)
    return;

  switch (coreCoder) {
    case CODEC_SAAC:

      WriteBits(hBitStream, ID_FIL, SI_ID_BITS_AAC);

      WriteBits(hBitStream, hSbrBit->sbrByteCnt, SI_FILL_COUNT_BITS);
      if (hSbrBit->sbrByteCnt == hSbrBit->cntEscValue) {
        WriteBits(hBitStream, hSbrBit->sbrExtByteCnt, SI_FILL_ESC_COUNT_BITS);
      }

      if (hCmonData->sbrCrcLen) {
        assert(8 * sizeof(crcBits) >= (unsigned)nCrcBits);

        WriteBits(hBitStream, AAC_SI_FIL_SBR_CRC, SI_FILL_EXTENTION_BITS);

        crcBitsStartPosition = SetWritePointer(hBitStream, 0);

        WriteBits(hBitStream, 0, nCrcBits);

        crcCalcStartPosition = SetWritePointer(hBitStream, 0);
      } else {
        WriteBits(hBitStream, AAC_SI_FIL_SBR, SI_FILL_EXTENTION_BITS);
      }
      break;

    default:
      assert(0);
      return;
  }

  ngsCopyBits(hBitStream, hCmonData->hSbrBitbuf, hCmonData->sbrHdrBits + hCmonData->sbrDataBits);

  WriteBits(hBitStream, 0, hSbrBit->fillBits);

  if (hCmonData->sbrCrcLen) {
    const unsigned short crcReg = 0x0;
    const unsigned short crcPoly = 0x233;
    const unsigned short crcMask = 0x200;

    int initialReadPointer = SetReadPointer(hBitStream, 0);
    int tmpPointer = initialReadPointer;
    int delta = 0;

    if (crcCalcStartPosition < tmpPointer) {
      tmpPointer -= GetBitBufSize(hBitStream);
    }
    delta = crcCalcStartPosition - tmpPointer;
    SetReadPointer(hBitStream, delta);

    crcBits = crcCalc(crcReg,
                      crcPoly,
                      crcMask,
                      hCmonData->sbrHdrBits + hCmonData->sbrDataBits + hSbrBit->fillBits,
                      hBitStream);

    crcBits &= (1 << nCrcBits) - 1;

    UpdateBitstreamField(hBitStream, crcBitsStartPosition, crcBits, nCrcBits);

    tmpPointer = SetReadPointer(hBitStream, 0);
    if (tmpPointer < initialReadPointer) {
      tmpPointer += GetBitBufSize(hBitStream);
    }
    delta = initialReadPointer - tmpPointer;
    SetReadPointer(hBitStream, delta);

    hCmonData->sbrCrcLen = hCmonData->sbrHdrBits + hCmonData->sbrDataBits + hSbrBit->fillBits;

    assert(initialReadPointer == SetReadPointer(hBitStream, 0));
  }

  hCmonData->sbrHdrBits = 0;
  hCmonData->sbrDataBits = 0;
}

int WriteSbrExtensionData(HANDLE_BIT_BUF hBitStream,
                          HANDLE_COMMON_DATA hCmonData,
                          SBR_LAYER sbrLayer,
                          int bCrcFlag,
                          int bWriteFillBits,
                          int nBitsExtensionType,
                          int byteAlignment,
                          CODEC_TYPE coreCodec) {
  int nBitsWritten = 0;
  int bByteAligned = 0;

  switch (coreCodec) {
    case CODEC_SAAC:
      bByteAligned = bWriteFillBits;
      break;
    default:
      bByteAligned = byteAlignment;
      break;
  }

  if (hCmonData != NULL) {
    int nSbrDataBits = 0;
    int nFillBits;

    const int nCrcBits = 10;
    int crcBitsStartPosition = 0, crcCalcStartPosition = 0;
    int crcBits;

    if (bCrcFlag) {
      assert(8 * sizeof(crcBits) >= (unsigned)nCrcBits);

      crcBitsStartPosition = SetWritePointer(hBitStream, 0);

      nBitsWritten += WriteBits(hBitStream, 0, nCrcBits);

      crcCalcStartPosition = SetWritePointer(hBitStream, 0);
    }

    switch (sbrLayer) {
      case SBR_NOT_SCALABLE:
      case SBR_MONO_BASE:
      case SBR_STEREO_BASE:

        nSbrDataBits = hCmonData->sbrHdrBits + hCmonData->sbrDataBits;
        ngsCopyBits(hBitStream, hCmonData->hSbrBitbuf, nSbrDataBits);
        nBitsWritten += nSbrDataBits;
        break;
      case SBR_STEREO_ENHANCE:

        nSbrDataBits = hCmonData->sbrDataBitsEnh;
        ngsCopyBits(hBitStream, hCmonData->hSbrBitbufEnh, nSbrDataBits);
        nBitsWritten += nSbrDataBits;
        break;
      case SBR_NO_DATA:
      default:
        break;
    }

    if (bByteAligned && ((nBitsWritten + nBitsExtensionType) % 8) != 0) {
      nFillBits = 8 - (nBitsWritten + nBitsExtensionType) % 8;
    } else {
      nFillBits = 0;
    }

    nBitsWritten += WriteBits(hBitStream, 0, nFillBits);

    if (bCrcFlag) {
      const unsigned short crcReg = 0x0;
      const unsigned short crcPoly = 0x233;
      const unsigned short crcMask = 0x200;

      int initialReadPointer = SetReadPointer(hBitStream, 0);
      int tmpPointer = initialReadPointer;
      int delta = 0;

      if (crcCalcStartPosition < tmpPointer) {
        tmpPointer -= GetBitBufSize(hBitStream);
      }
      delta = crcCalcStartPosition - tmpPointer;
      SetReadPointer(hBitStream, delta);

      crcBits = crcCalc(crcReg,
                        crcPoly,
                        crcMask,
                        nSbrDataBits + nFillBits,
                        hBitStream);

      crcBits &= (1 << nCrcBits) - 1;

      UpdateBitstreamField(hBitStream, crcBitsStartPosition, crcBits, nCrcBits);

      tmpPointer = SetReadPointer(hBitStream, 0);
      if (tmpPointer < initialReadPointer) {
        tmpPointer += GetBitBufSize(hBitStream);
      }
      delta = initialReadPointer - tmpPointer;
      SetReadPointer(hBitStream, delta);

      hCmonData->sbrCrcLen = nSbrDataBits + nFillBits;

      assert(initialReadPointer == SetReadPointer(hBitStream, 0));
    }
  }

  return nBitsWritten;
}
