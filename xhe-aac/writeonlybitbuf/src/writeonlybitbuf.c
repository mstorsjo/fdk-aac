
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

#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "writeonlybitbuf.h"

static unsigned char GetMask_UnsignedChar(int N) {
  int i;
  unsigned char mask = 0;

  assert(N >= 0);
  assert((unsigned int)N <= CHAR_BIT * sizeof(mask));

  for (i = 0; i < N; i++) {
    if ((unsigned int)i >= CHAR_BIT * sizeof(mask)) break;
    mask = (mask << 1) + 1;
  }

  return mask;
}

static unsigned int GetMask_UnsignedInt(int N) {
  int i;
  unsigned int mask = 0;

  assert(N >= 0);
  assert((unsigned int)N <= CHAR_BIT * sizeof(mask));

  for (i = 0; i < N; i++) {
    if ((unsigned int)i >= CHAR_BIT * sizeof(mask)) break;
    mask = (mask << 1) + 1;
  }

  return mask;
}

static unsigned char getSetValue(int value, int nBits) {
  unsigned char tmp = 0;

  if (value) {
    tmp = GetMask_UnsignedChar(nBits);
  }

  return tmp;
}

static void WriteByte(wobitbufHandle self,
                      unsigned char data) {
  assert(self->isInit == 0xdeadbeef);
  if (self->bitPos) {
    int restBits = self->bitPos;
    int bitsToFill = 8 - restBits;
    unsigned char bs = self->charBuffer[self->charBufPos];
    unsigned char tmp = '\0';

    tmp |= (data >> restBits);
    bs >>= bitsToFill;
    bs <<= bitsToFill;
    bs |= tmp;
    self->charBuffer[self->charBufPos] = bs;
    self->charBufPos++;

    tmp = ((data & (0x000000FFU >> bitsToFill)) << bitsToFill);
    bs = self->charBuffer[self->charBufPos] & GetMask_UnsignedChar(bitsToFill);
    bs |= tmp;
    self->charBuffer[self->charBufPos] = bs;
  } else {
    self->charBuffer[self->charBufPos] = data;
    self->charBufPos++;
  }

  self->bitsWritten += 8;
}

static void WriteBitsUnderEight(wobitbufHandle self,
                                unsigned char data,
                                int nbits) {
  unsigned char bs;
  unsigned char tmp = '\0';
  assert(self->isInit == 0xdeadbeef);
  if (self->bitPos + nbits >= 8) {
    int bitsToFill = 8 - self->bitPos;
    bs = self->charBuffer[self->charBufPos];

    tmp |= ((data >> (nbits - bitsToFill)) & GetMask_UnsignedChar(bitsToFill));
    bs >>= bitsToFill;
    bs <<= bitsToFill;
    bs |= tmp;
    self->charBuffer[self->charBufPos] = bs;
    self->charBufPos++;
    self->bitPos = 0;
    nbits -= bitsToFill;
    self->bitsWritten += bitsToFill;
  }

  if (nbits) {
    unsigned char help_bs;
    int offset = self->bitPos;

    bs = self->charBuffer[self->charBufPos];

    tmp = (data & GetMask_UnsignedChar(nbits)) << (8 - nbits - offset);
    help_bs = bs & GetMask_UnsignedChar(8 - nbits - offset);
    bs >>= (8 - offset);
    bs <<= (8 - offset);
    bs |= (tmp | help_bs);
    self->charBuffer[self->charBufPos] = bs;
    self->bitPos += nbits;
    self->bitsWritten += nbits;
  }
}

static int NthPowerOfTwo(unsigned int const N, unsigned int *const NthPowerOfTwo) {
  const unsigned int maxN = sizeof(unsigned int) * CHAR_BIT;

  if (!NthPowerOfTwo) {
    assert(0);
    return 1;
  } else if (N > maxN) {
    assert(0);
    return 1;
  }

  if (N == maxN) {
    *NthPowerOfTwo = UINT_MAX;
  } else {
    *NthPowerOfTwo = (1u << N) - 1;
  }

  return 0;
}

int WOBITBUFAPI wobitbuf_Init(
    wobitbufHandle self,
    unsigned char *const extBuffer,
    int extBufSize,
    int startPos) {
  int err = 0;

  if (self == NULL || extBuffer == NULL || extBufSize <= 0 || startPos < 0) {
    assert(0);
    err = 1;
  }

  if (!err) {
    self->charBuffer = extBuffer;
    self->charBufSize = extBufSize;

    self->charBufPos = startPos >> 3;
    self->bitPos = startPos % 8;

    self->startPos = startPos;
    self->endPos = startPos + (extBufSize - 1);

    self->bitsWritten = 0;
    self->isInit = 0xdeadbeef;
  }

  return err;
}

void WOBITBUFAPI wobitbuf_Reset(
    wobitbufHandle self) {
  assert(self);

  if (self != NULL) {
    assert(self->isInit == 0xdeadbeef);

    self->charBufPos = self->startPos >> 3;
    self->bitPos = self->startPos % 8;
    self->bitsWritten = 0;
  }
}

void WOBITBUFAPI wobitbuf_Copy(
    wobitbufHandle desHandle,
    wobitbufHandle refHandle) {
  assert(desHandle && refHandle);

  if (desHandle != NULL && refHandle != NULL) {
    assert(refHandle->isInit == 0xdeadbeef);
    *desHandle = *refHandle;
  }
}

int WOBITBUFAPI wobitbuf_GetBufferSize(
    wobitbufHandle self) {
  int bufsize = 0;

  assert(self);

  if (self != NULL) {
    assert(self->isInit == 0xdeadbeef);
    bufsize = self->charBufSize;
  }

  return bufsize;
}

int WOBITBUFAPI wobitbuf_WriteBits(
    wobitbufHandle self,
    unsigned int data,
    int nBits) {
  int err = 0;

  assert(nBits <= 32);

  if (self == NULL || (wobitbuf_GetBitsAvail(self) - nBits < 0)) {
    assert(0);
    err = 1;
  }

  if (!err && nBits > 0 && nBits <= 32) {
    assert(self->isInit == 0xdeadbeef);

    while (nBits >= 8) {
      unsigned char tmp = '\0';
      nBits += -8;
      tmp |= (unsigned char)(data >> nBits);
      WriteByte(self, tmp);
    }

    if (nBits) {
      unsigned char tmp = (unsigned char)(data & GetMask_UnsignedInt(nBits));
      WriteBitsUnderEight(self, tmp, nBits);
    }
  }

  return err;
}

int WOBITBUFAPI wobitbuf_WriteBytes(
    wobitbufHandle self,
    unsigned char const *const srcBuffer,
    int nBytes) {
  int byteCount;
  int err = 0;

  if (self == NULL || (wobitbuf_GetBitsAvail(self) - (nBytes << 3) < 0)) {
    assert(0);
    err = 1;
  }

  if (!err) {
    assert(self->isInit == 0xdeadbeef);
    for (byteCount = 0; byteCount < nBytes; byteCount++) {
      WriteByte(self, srcBuffer[byteCount]);
    }
  }

  return err;
}

int WOBITBUFAPI wobitbuf_WritePartialBytes(
    wobitbufHandle self,
    unsigned char const *data,
    int nBits) {
  int err = 0;

  if (self == NULL || (wobitbuf_GetBitsAvail(self) - nBits < 0)) {
    assert(0);
    err = 1;
  }

  if (!err && nBits > 0) {
    assert(self->isInit == 0xdeadbeef);

    while (nBits >= 8) {
      nBits += -8;
      WriteByte(self, *data);
      data++;
    }

    if (nBits) {
      WriteBitsUnderEight(self, *data >> (8 - nBits), nBits);
    }
  }

  return err;
}

int WOBITBUFAPI wobitbuf_Seek(
    wobitbufHandle self,
    int nBits) {
  int err = 0;

  if (self != NULL) {
    int newPos = ((self->charBufPos << 3) + self->bitPos) + nBits;

    assert(self->isInit == 0xdeadbeef);

    if (newPos <= self->endPos && newPos >= self->startPos) {
      self->charBufPos = newPos >> 3;
      self->bitPos = newPos % 8;
    } else {
      assert(0);
      err = 1;
    }
  } else {
    assert(0);
    err = 1;
  }

  return err;
}

void WOBITBUFAPI wobitbuf_ByteAlign(
    wobitbufHandle self) {
  assert(self);

  if (self != NULL) {
    int startbitpos = self->startPos % 8;
    int bitPos = self->bitPos;
    int bitsToWrite = ((8 - bitPos) + startbitpos) % 8;

    assert(self->isInit == 0xdeadbeef);

    if (wobitbuf_GetBitsAvail(self) >= bitsToWrite) {
      if (self->bitPos + bitsToWrite >= 8) {
        self->charBufPos++;
      }

      self->bitPos = (self->bitPos + bitsToWrite) % 8;
      self->bitsWritten += bitsToWrite;
    }
  }
}

void WOBITBUFAPI wobitbuf_ByteAlignSet(
    wobitbufHandle self,
    int value) {
  assert(self);
  assert(value == 0 || value == 1);
  if (self != NULL) {
    int startbitpos = self->startPos % 8;
    int bitPos = self->bitPos;
    int bitsToWrite = (8 - bitPos) + startbitpos;

    assert(self->isInit == 0xdeadbeef);

    if (bitsToWrite != 8 && bitsToWrite <= wobitbuf_GetBitsAvail(self)) {
      WriteBitsUnderEight(self, getSetValue(value, bitsToWrite), bitsToWrite);
    }
  }
}

int WOBITBUFAPI wobitbuf_GetBitsAvail(
    wobitbufHandle self) {
  int bitsAvail = 0;

  assert(self);

  if (self != NULL) {
    assert(self->isInit == 0xdeadbeef);
    bitsAvail = self->charBufSize - wobitbuf_GetBufPos(self);
  }

  return bitsAvail;
}

int WOBITBUFAPI wobitbuf_GetBufPos(
    wobitbufHandle self) {
  int bitsFilled = 0;

  assert(self);

  if (self != NULL) {
    assert(self->isInit == 0xdeadbeef);
    bitsFilled = ((self->charBufPos << 3) + self->bitPos) - self->startPos;
  }

  return bitsFilled;
}

int WOBITBUFAPI wobitbuf_GetBitsWritten(
    wobitbufHandle self) {
  if (self == NULL) {
    assert(0);
    return 0;
  }
  assert(self->isInit == 0xdeadbeef);
  return self->bitsWritten;
}

int WOBITBUFAPI wobitbuf_WriteEscapedValue(
    wobitbufHandle self,
    unsigned int data,
    int nBits[NUMBER_OF_N_BITS_FOR_ESCAPED_VALUE]) {
  int err = 0;

  unsigned int escVal[NUMBER_OF_N_BITS_FOR_ESCAPED_VALUE];

  if (self == NULL || nBits == NULL) {
    assert(0);
    err = 1;
  }
  assert(self->isInit == 0xdeadbeef);

  if (!err) {
    int i;
    for (i = 0; i < NUMBER_OF_N_BITS_FOR_ESCAPED_VALUE; i++) {
      if (nBits[i] < 0 || 0 != NthPowerOfTwo((unsigned int)nBits[i], &escVal[i])) {
        assert(0);
        err = 1;
        break;
      }
    }
  }

  if (!err) {
    int cnt = 0;
    int iterate = 0;
    do {
      if (data < escVal[cnt]) {
        err = wobitbuf_WriteBits(self, data, nBits[cnt]);
        data = 0;
        iterate = 0;
      } else {
        err = wobitbuf_WriteBits(self, escVal[cnt], nBits[cnt]);
        data -= escVal[cnt];
        iterate = 1;
      }

      cnt++;
      if (err) break;
    } while (iterate && (cnt < NUMBER_OF_N_BITS_FOR_ESCAPED_VALUE));
  }

  if (!err) {
    if (data) {
      assert(0);
      err = 1;
    }
  }

  return err;
}
