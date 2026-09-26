
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

#include "readonlybitbuf.h"
#include <stdio.h>

static unsigned int GetMask(int size) {
  return (1 << size) - 1;
}

static unsigned int GetByte(robitbufHandle self) {
  unsigned int tmp = 0;
  int byteSize, bitSize, bitRest;

  if (self->charBuffer == NULL) return 0;

  bitSize = self->charBufSize + self->startPos;

  byteSize = (bitSize + 7) >> 3;
  bitRest = bitSize - 8 * (byteSize - 1);

  if ((self->charBufPos >= 0) && (self->charBufPos < byteSize)) {
    tmp = self->charBuffer[self->charBufPos];

    if ((self->charBufPos == (byteSize - 1)) && (bitRest != 8)) {
      tmp &= (GetMask(bitRest) << (8 - bitRest));
    }
  }

  self->charBufPos++;

  return tmp;
}

void ROBITBUFAPI robitbuf_Init(
    robitbufHandle self,
    const unsigned char *extBuffer,
    int extBufSize,
    int startPos) {
  if (self == NULL) return;

  self->charBuffer = NULL;
  self->charBufSize = 0;
  self->charBufPos = 0;
  self->startPos = startPos;

  self->cache = 0;
  self->bitsInCache = -startPos;

  self->bitsRead = 0;

  self->charBuffer = extBuffer;
  self->charBufSize = (extBufSize > 0) ? extBufSize : 0;
}

void ROBITBUFAPI robitbuf_Reset(robitbufHandle self) {
  if (self == NULL) return;

  self->charBufPos = 0;

  self->cache = 0;
  self->bitsInCache = -self->startPos;

  self->bitsRead = 0;
}

void ROBITBUFAPI robitbuf_Copy(
    robitbufHandle desHandle,
    robitbufHandle refHandle) {
  if (desHandle == NULL) return;
  if (refHandle == NULL) return;

  desHandle->charBuffer = refHandle->charBuffer;
  desHandle->charBufSize = refHandle->charBufSize;
  desHandle->charBufPos = refHandle->charBufPos;
  desHandle->startPos = refHandle->startPos;

  desHandle->cache = refHandle->cache;
  desHandle->bitsInCache = refHandle->bitsInCache;

  desHandle->bitsRead = refHandle->bitsRead;
}

int ROBITBUFAPI robitbuf_GetBufferSize(robitbufHandle self) {
  if (self == NULL) return 0;

  return self->charBufSize;
}

unsigned int ROBITBUFAPI robitbuf_ReadBits(
    robitbufHandle self,
    int nBits) {
  unsigned int tmp = 0;
  int bitsToRead, bitsToDrop, restBits;

  if (self == NULL) return 0;

  if (nBits > 32) {
    bitsToDrop = nBits - 32;
    self->bitsRead += bitsToDrop;
    self->bitsInCache -= bitsToDrop;
    nBits -= bitsToDrop;
  }

  while (nBits > 0) {
    while (self->bitsInCache <= -8) {
      self->bitsInCache += 8;
      self->charBufPos++;
    }

    if (self->bitsInCache < 16) {
      unsigned int tmpHi, tmpLo;

      self->cache = (self->cache & 0x0000FFFFU) << 16;
      tmpHi = GetByte(self);
      tmpLo = GetByte(self);
      self->cache |= ((tmpHi << 8) | tmpLo);
      self->bitsInCache += 16;
    }

    if (nBits > self->bitsInCache)
      bitsToRead = self->bitsInCache;
    else
      bitsToRead = nBits;

    restBits = self->bitsInCache - bitsToRead;

    tmp <<= bitsToRead;

    tmp |= ((self->cache >> restBits) & GetMask(bitsToRead));

    nBits -= bitsToRead;
    self->bitsInCache -= bitsToRead;

    self->bitsRead += bitsToRead;
  }

  return tmp;
}

void ROBITBUFAPI robitbuf_ReadBytes(
    robitbufHandle self,
    unsigned char *desBuffer,
    int nBytes) {
  int i;

  if (self == NULL) return;

  for (i = 0; i < nBytes; i++) {
    desBuffer[i] = (unsigned char)robitbuf_ReadBits(self, 8);
  }
}

void ROBITBUFAPI robitbuf_PushBack(
    robitbufHandle self,
    int nBits) {
  int leftBits = 0;
  int leftBytes = 0;

  if (self == NULL) return;

  self->bitsRead -= nBits;

  nBits += self->bitsInCache;
  self->bitsInCache = 0;

  if (nBits >= 0)
    leftBytes = nBits / 8 + 1;
  else
    leftBytes = nBits / 8;

  leftBits = leftBytes * 8 - nBits;

  self->charBufPos -= leftBytes;

  self->bitsInCache -= leftBits;
}

unsigned int ROBITBUFAPI robitbuf_ParseEscapedBitValue(
    robitbufHandle bitbuf,
    unsigned int nBits1,
    unsigned int nBits2,
    unsigned int nBits3) {
  unsigned int valueAdd = 0;
  unsigned int maxValue1 = (1 << nBits1) - 1;
  unsigned int maxValue2 = (1 << nBits2) - 1;
  unsigned int value = robitbuf_ReadBits(bitbuf, (int)nBits1);

  if (value == maxValue1) {
    valueAdd = robitbuf_ReadBits(bitbuf, (int)nBits2);

    value += valueAdd;

    if (valueAdd == maxValue2) {
      valueAdd = robitbuf_ReadBits(bitbuf, (int)nBits3);
      value += valueAdd;
    }
  }

  return value;
}

void ROBITBUFAPI robitbuf_ByteAlign(robitbufHandle self) {
  int bitsToDrop = 0;

  if (self == NULL) return;

  if (self->bitsInCache >= 0)
    bitsToDrop = self->bitsInCache % 8;
  else
    bitsToDrop = (8 - ((-self->bitsInCache) % 8)) % 8;

  self->bitsInCache -= bitsToDrop;

  self->bitsRead += bitsToDrop;
}

int ROBITBUFAPI robitbuf_ByteAlignDbg(robitbufHandle self, char debchar) {
  int bitsToDrop = 0;

  (void)debchar;

  if (self == NULL) return 0;

  if (self->bitsInCache >= 0)
    bitsToDrop = self->bitsInCache % 8;
  else
    bitsToDrop = (8 - ((-self->bitsInCache) % 8)) % 8;

  return bitsToDrop;
}

int ROBITBUFAPI robitbuf_GetBitsAvail(robitbufHandle self) {
  if (self == NULL) return -1;

  return self->charBufSize - self->bitsRead;
}

int ROBITBUFAPI robitbuf_GetBitsRead(robitbufHandle self) {
  if (self == NULL) return -1;

  return self->bitsRead;
}
