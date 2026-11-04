
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
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "bit_buf.h"

typedef struct {
  CRC_ID id;
  int start;
  int end;
} CRC_TAB;

typedef struct bitbuffer {
  int size;
  bbWord *buffer;

  bbWord rCache;
  bbWord wCache;

  signed int wBitsLeft;
  signed int rBitsLeft;

  int wIdx;
  int rIdx;

  int bitsAvail;
  int bitcnt;

  int rCacheDirty;

  int crcFlag;
  CRC_TAB crcTab[CRCTAB_MAX_ENTRIES];

  int AuReady;
  int AuLengthBits[MAX_AU_IN_BUFFER];
} BIT_BUF;

enum {
  bbWordSize = 8 * sizeof(bbWord)
};

static const unsigned int powof2[] =
    {
        1u << 0, 1u << 1, 1u << 2, 1u << 3, 1u << 4, 1u << 5, 1u << 6, 1u << 7,
        1u << 8, 1u << 9, 1u << 10, 1u << 11, 1u << 12, 1u << 13, 1u << 14, 1u << 15,
        1u << 16, 1u << 17, 1u << 18, 1u << 19, 1u << 20, 1u << 21, 1u << 22, 1u << 23,
        1u << 24, 1u << 25, 1u << 26, 1u << 27, 1u << 28, 1u << 29, 1u << 30, 1u << 31,
        0};

static void bbInit(struct bitbuffer *pBuffer, int size) {
  assert(2 * bbWordSize <= 8 * sizeof(unsigned int));

  if (pBuffer != NULL) {
    pBuffer->size = (size + bbWordSize - 1) / bbWordSize;
    pBuffer->buffer = (bbWord *)iisCalloc(pBuffer->size,
                                          bbWordSize / 8);

    pBuffer->rCache = 0;
    pBuffer->wCache = 0;

    pBuffer->rBitsLeft = 0;
    pBuffer->wBitsLeft = bbWordSize;

    pBuffer->rIdx = -1;
    pBuffer->wIdx = 0;

    pBuffer->bitsAvail = 0;
    pBuffer->bitcnt = 0;

    pBuffer->rCacheDirty = 0;

    pBuffer->crcFlag = 0;
    InitCrcTab(pBuffer);

    pBuffer->AuReady = 0;
  }
}

static void bbDestroy(struct bitbuffer *pBuffer) {
  if (pBuffer && pBuffer->buffer) iisFree(pBuffer->buffer);
}

static void bbResetStates(struct bitbuffer *pBuffer) {
  assert(2 * bbWordSize <= 8 * sizeof(unsigned int));

  if (pBuffer != NULL) {
    pBuffer->rCache = 0;
    pBuffer->wCache = 0;

    pBuffer->rBitsLeft = 0;
    pBuffer->wBitsLeft = bbWordSize;

    pBuffer->rIdx = -1;
    pBuffer->wIdx = 0;

    pBuffer->bitsAvail = 0;
    pBuffer->bitcnt = 0;

    pBuffer->rCacheDirty = 0;

    pBuffer->crcFlag = 0;
    InitCrcTab(pBuffer);

    pBuffer->AuReady = 0;
  }
}
static void
bbWriteBitsShort(struct bitbuffer *pBuffer,
                 bbWord w,
                 unsigned int len) {
  if (pBuffer != NULL) {
    unsigned int wCache;
    unsigned int mask;

    assert(len <= bbWordSize);

    pBuffer->bitsAvail += len;
    pBuffer->bitcnt = (pBuffer->bitcnt + (int)len) % 8;

    mask = (1 << len) - 1;
    wCache = pBuffer->wCache;
    wCache <<= len;
    wCache |= (w & mask);

    pBuffer->wBitsLeft -= (signed int)len;

    pBuffer->rCacheDirty |= (pBuffer->wIdx == pBuffer->rIdx);

    if (pBuffer->wBitsLeft >= 0) {
      pBuffer->wCache = (bbWord)wCache;
    } else {
      unsigned int t;

      t = wCache >> (-pBuffer->wBitsLeft);
      wCache ^= t << (-pBuffer->wBitsLeft);
      pBuffer->buffer[pBuffer->wIdx] = (bbWord)t;

      if (++(pBuffer->wIdx) == pBuffer->size)
        pBuffer->wIdx = 0;

      pBuffer->wBitsLeft += bbWordSize;
      pBuffer->wCache = (bbWord)wCache;
    }
  }
}

static void bbSyncWriteCache(struct bitbuffer *pBuffer) {
  if (pBuffer != NULL) {
    bbWord dstmask;

    dstmask = (bbWord) ~((powof2[bbWordSize - pBuffer->wBitsLeft] - 1) << pBuffer->wBitsLeft);

    pBuffer->buffer[pBuffer->wIdx] &= dstmask;
    pBuffer->buffer[pBuffer->wIdx] |= (pBuffer->wCache << pBuffer->wBitsLeft);
  }
}

static bbWord bbReadBitsShort(struct bitbuffer *pBuffer, int len) {
  unsigned int rc = 0;
  if (pBuffer != NULL) {
    assert(len <= (int)bbWordSize);
    assert(len <= pBuffer->bitsAvail);

    pBuffer->bitsAvail -= len;

    if (pBuffer->rCacheDirty) {
      unsigned int temp;
      bbSyncWriteCache(pBuffer);

      temp = (unsigned int)pBuffer->buffer[pBuffer->rIdx];
      temp <<= (bbWordSize - pBuffer->rBitsLeft);
      temp &= 0xFFFFU;
      pBuffer->rCache = (bbWord)temp;
      pBuffer->rCacheDirty = 0;
    }

    rc = (pBuffer->rCache >> (bbWordSize - len));

    if (len <= pBuffer->rBitsLeft) {
      pBuffer->rCache = (bbWord)(pBuffer->rCache << len);
      pBuffer->rBitsLeft -= len;
    } else {
      len -= pBuffer->rBitsLeft;

      if (++(pBuffer->rIdx) == pBuffer->size) pBuffer->rIdx = 0;

      if (pBuffer->rIdx == pBuffer->wIdx) {
        bbSyncWriteCache(pBuffer);
      }

      pBuffer->rCache = pBuffer->buffer[pBuffer->rIdx];
      pBuffer->rCacheDirty = 0;

      rc |= (pBuffer->rCache >> (bbWordSize - len));
      pBuffer->rCache = (bbWord)((unsigned int)pBuffer->rCache << len);

      pBuffer->rBitsLeft = bbWordSize - len;
    }
  }

  return rc;
}

static bbWord bbReadWord(struct bitbuffer *pBuffer) {
  bbWord readWord = 0x0;

  if (pBuffer != NULL) {
    unsigned int ba = pBuffer->bitsAvail;
    if (bbWordSize > ba) {
      return bbReadBitsShort(pBuffer, pBuffer->bitsAvail) << (bbWordSize - ba);
    }

    pBuffer->bitsAvail -= bbWordSize;

    if (pBuffer->rBitsLeft == 0) {
      if (++(pBuffer->rIdx) == pBuffer->size)
        pBuffer->rIdx = 0;
    } else {
      assert(pBuffer->rBitsLeft == bbWordSize);
      pBuffer->rBitsLeft = 0;
      pBuffer->rCache = 0x0;
    }
    readWord = pBuffer->buffer[pBuffer->rIdx];
  }

  return readWord;
}

static void bbWriteWord(struct bitbuffer *pBuffer, bbWord dataWord) {
  if (pBuffer != NULL) {
    pBuffer->bitsAvail += bbWordSize;
    pBuffer->bitcnt += bbWordSize % 8;

    pBuffer->rCacheDirty |= (pBuffer->wIdx == pBuffer->rIdx);

    if (pBuffer->wBitsLeft == 0) {
      pBuffer->buffer[pBuffer->wIdx] = pBuffer->wCache;
      pBuffer->wCache = dataWord;

      if (++(pBuffer->wIdx) == pBuffer->size)
        pBuffer->wIdx = 0;

    } else {
      assert(pBuffer->wBitsLeft == bbWordSize);
      pBuffer->wCache = dataWord;
      pBuffer->wBitsLeft = 0;
    }
  }
}

static int bbByteAlign(struct bitbuffer *pBuffer) {
  int bits = 0;

  if (pBuffer != NULL) {
    bits = (8 - (pBuffer->bitcnt % 8)) % 8;
    bbWriteBitsShort(pBuffer, 0, bits);

    pBuffer->bitcnt = 0;
  }

  return bits;
}

HANDLE_BIT_BUF CreateBitBuffer(int bitSize) {
  HANDLE_BIT_BUF hBitbuf = (HANDLE_BIT_BUF)iisMalloc(sizeof(struct bitbuffer));

  bbInit((struct bitbuffer *)hBitbuf, bitSize);
  return hBitbuf;
}

void DeleteBitBuffer(HANDLE_BIT_BUF hBitBuf) {
  if (hBitBuf) {
    bbDestroy(hBitBuf);
    iisFree(hBitBuf);
  }
}

void ResetBitBuffer(HANDLE_BIT_BUF hBitBuf) {
  if (hBitBuf)
    bbResetStates(hBitBuf);
}

int GetBitBufSize(HANDLE_BIT_BUF hBitBuf) {
  return (hBitBuf->size * bbWordSize);
}

int SetReadPointer(HANDLE_BIT_BUF hBitBuf, int delta) {
  int nOldpos = 0;
  int tmp;

  if (hBitBuf != NULL) {
    nOldpos = hBitBuf->rIdx * bbWordSize + (bbWordSize - hBitBuf->rBitsLeft);

    if (delta == 0) return nOldpos;

    tmp = (hBitBuf->rIdx * bbWordSize + delta + (bbWordSize - hBitBuf->rBitsLeft));
    if (tmp > (int)(hBitBuf->size * bbWordSize - 1)) {
      tmp -= hBitBuf->size * bbWordSize;
    }

    if (tmp < 0) {
      tmp += hBitBuf->size * bbWordSize;
    }

    hBitBuf->rIdx = tmp / bbWordSize;

    if (hBitBuf->wIdx == hBitBuf->rIdx)
      bbSyncWriteCache(hBitBuf);

    hBitBuf->rCache = hBitBuf->buffer[hBitBuf->rIdx];

    hBitBuf->rBitsLeft = bbWordSize - tmp % bbWordSize;

    hBitBuf->rCache <<= (bbWordSize - hBitBuf->rBitsLeft);

    hBitBuf->bitsAvail -= delta;
  }

  return nOldpos;
}

void ByteAlignReadPointer(HANDLE_BIT_BUF hBitBuf) {
  int alignBits;

  if (hBitBuf != NULL) {
    alignBits = GetBitsAvail(hBitBuf) % 8;

    if (alignBits) {
      ReadBits(hBitBuf, alignBits);
    }
  }
}

int GetWritePointer(HANDLE_BIT_BUF hBitBuf) {
  int nOldpos = 0;

  if (hBitBuf != NULL) {
    nOldpos = hBitBuf->wIdx * bbWordSize + (bbWordSize - hBitBuf->wBitsLeft);
  }

  return nOldpos;
}

int SetWritePointer(HANDLE_BIT_BUF hBitBuf, int delta) {
  int nOldpos = 0;

  int tmp;

  if (hBitBuf != NULL) {
    nOldpos = hBitBuf->wIdx * bbWordSize + (bbWordSize - hBitBuf->wBitsLeft);

    if (delta == 0) return nOldpos;

    bbSyncWriteCache(hBitBuf);

    tmp = (hBitBuf->wIdx * bbWordSize + delta + (bbWordSize - hBitBuf->wBitsLeft));

    if (tmp > (int)(hBitBuf->size * bbWordSize - 1)) {
      tmp -= hBitBuf->size * bbWordSize;
    }
    if (tmp < 0) {
      tmp += hBitBuf->size * bbWordSize;
    }

    hBitBuf->wIdx = tmp / bbWordSize;

    hBitBuf->wCache = hBitBuf->buffer[hBitBuf->wIdx];

    hBitBuf->wBitsLeft = bbWordSize - tmp % bbWordSize;

    hBitBuf->wCache >>= hBitBuf->wBitsLeft;

    hBitBuf->bitsAvail += delta;
    hBitBuf->bitcnt += delta;
  }

  return nOldpos;
}

int GetBitsAvail(HANDLE_BIT_BUF hBitBuf) {
  int nBitsAvail = 0;

  if (hBitBuf != NULL) {
    nBitsAvail = hBitBuf->bitsAvail;
  }

  return nBitsAvail;
}

BIT_BUF_WORD ReadBits(HANDLE_BIT_BUF hBitBuf, int nBits) {
  BIT_BUF_WORD rc;
  if (nBits < (int)bbWordSize) {
    rc = bbReadBitsShort(hBitBuf, nBits);
  } else {
    assert(bbWordSize * 2 <= sizeof(BIT_BUF_WORD) * 8);

    rc = bbReadBitsShort(hBitBuf, bbWordSize);
    rc <<= nBits - bbWordSize;
    rc |= bbReadBitsShort(hBitBuf, nBits - bbWordSize);
  }

  return rc;
}

void ReadBytes(struct bitbuffer *pBuffer, unsigned char *dst, int n) {
  int i;
  int ba;

  if ((dst != NULL) && (pBuffer != NULL)) {
    if (pBuffer->rBitsLeft % 8) {
      for (i = 0; i < n; i++) {
        ba = pBuffer->bitsAvail;
        if (ba < 8) {
          dst[i] = (unsigned char)bbReadBitsShort(pBuffer, ba);
          dst[i] <<= (8 - ba);
        } else {
          dst[i] = (unsigned char)bbReadBitsShort(pBuffer, 8);
        }
      }
    } else {
      assert(bbWordSize == 16);

      for (i = 0; i < n && (pBuffer->rBitsLeft % bbWordSize); i++) {
        dst[i] = (unsigned char)bbReadBitsShort(pBuffer, min(8, pBuffer->bitsAvail));
      }

      bbSyncWriteCache(pBuffer);

      for (; i <= (int)(n - bbWordSize / 8); i += bbWordSize / 8) {
        bbWord t = bbReadWord(pBuffer);

        dst[i] = t >> 8;
        dst[i + 1] = t & 0xff;
      }

      for (; i < n; i++) {
        ba = pBuffer->bitsAvail;
        if (ba < 8) {
          dst[i] = (unsigned char)bbReadBitsShort(pBuffer, ba);
          dst[i] <<= (8 - ba);
        } else {
          dst[i] = (unsigned char)bbReadBitsShort(pBuffer, 8);
        }
      }
    }
  }
}

int WriteBits(HANDLE_BIT_BUF hBitBuf, BIT_BUF_WORD bValue, int nBits) {
  if (hBitBuf && nBits > 0) {
    if (nBits != 32) {
      assert(bValue >> nBits == 0);
    }

    assert(((int)hBitBuf->bitsAvail) + nBits <= hBitBuf->size * bbWordSize);

    if (nBits <= (int)bbWordSize) {
      bbWriteBitsShort(hBitBuf, (bbWord)bValue, nBits);
    } else {
      bbWriteBitsShort(hBitBuf, (bbWord)(bValue >> 16), nBits - bbWordSize);
      bbWriteBitsShort(hBitBuf, (bbWord)(bValue & 0xffff), bbWordSize);
    }
  }

  return nBits;
}

void WriteBytes(HANDLE_BIT_BUF hBitBuf, unsigned char const *source, int n) {
  int i;
  bbWord tmp;

  if ((source != NULL) && (hBitBuf != NULL)) {
    assert(((int)hBitBuf->bitsAvail) + (n << 3) <= hBitBuf->size * bbWordSize);

    if (hBitBuf->rBitsLeft % 8) {
      for (i = 0; i < n; i++) {
        bbWriteBitsShort(hBitBuf, (bbWord)source[i], 8);
      }
    } else {
      assert(bbWordSize == 16);

      for (i = 0; i < n && (hBitBuf->wBitsLeft % bbWordSize); i++) {
        bbWriteBitsShort(hBitBuf, (bbWord)source[i], 8);
      }

      for (; i <= (int)(n - bbWordSize / 8); i += bbWordSize / 8) {
        tmp = source[i] << 8;
        tmp |= source[i + 1];
        bbWriteWord(hBitBuf, tmp);
      }

      for (; i < n; i++) {
        bbWriteBitsShort(hBitBuf, (bbWord)source[i], 8);
      }
    }
  }
}

int ByteAlign(HANDLE_BIT_BUF hBitbuf) {
  return bbByteAlign(hBitbuf);
}

void ngsCopyBits(HANDLE_BIT_BUF hBitBufWrite, HANDLE_BIT_BUF hBitBufRead, int nBits) {
  if (hBitBufWrite != NULL) {
    assert(((int)hBitBufWrite->bitsAvail) + nBits <= hBitBufWrite->size * bbWordSize);
    while (nBits >= bbWordSize) {
      bbWriteBitsShort(hBitBufWrite, bbReadBitsShort(hBitBufRead, bbWordSize), bbWordSize);
      nBits -= bbWordSize;
    }
    while (nBits > 0) {
      bbWriteBitsShort(hBitBufWrite, bbReadBitsShort(hBitBufRead, 1), 1);
      nBits--;
    }
  }
}

void UpdateBitstreamField(HANDLE_BIT_BUF hBitStream, int position, int value,
                          int length) {
  int tmp, saveWritePos;

  saveWritePos = SetWritePointer(hBitStream, 0);

  if (saveWritePos >= position) {
    SetWritePointer(hBitStream, position - saveWritePos);
  } else {
    int bitBufSize = GetBitBufSize(hBitStream);
    SetWritePointer(hBitStream, position - (saveWritePos + bitBufSize));
  }

  WriteBits(hBitStream, value, length);
  tmp = SetWritePointer(hBitStream, 0);

  if (saveWritePos >= tmp) {
    SetWritePointer(hBitStream, saveWritePos - tmp);
  } else {
    int bitBufSize = GetBitBufSize(hBitStream);
    SetWritePointer(hBitStream, saveWritePos + bitBufSize - tmp);
  }
}

HANDLE_ERROR_INFO
IncAuReady(HANDLE_BIT_BUF bitBuffer, int AuLength) {
  HANDLE_ERROR_INFO errInfo = noError;

  int AuReady = bitBuffer->AuReady;

  if ((AuReady > (MAX_AU_IN_BUFFER - 1)) || (AuReady < 0)) {
    errInfo = iisUtil_ERROR(CDI, "Access Unit Buffer Overflow in IncAuReady");
  }

  bitBuffer->AuLengthBits[AuReady] = AuLength;

  bitBuffer->AuReady++;

  return errInfo;
}

int GetAuReady(HANDLE_BIT_BUF bitBuffer) {
  return bitBuffer->AuReady;
}

HANDLE_ERROR_INFO
DecAuReady(HANDLE_BIT_BUF bitBuffer, int *pNextAULen) {
  int i;

  HANDLE_ERROR_INFO errInfo = noError;

  int AuReady = bitBuffer->AuReady;

  if ((AuReady > MAX_AU_IN_BUFFER) || (AuReady <= 0)) {
    errInfo = iisUtil_ERROR(CDI, "Access Unit Buffer Underflow in DecAuReady");
  }

  *pNextAULen = bitBuffer->AuLengthBits[0];

  for (i = 0; i < (AuReady - 1); i++)
    bitBuffer->AuLengthBits[i] = bitBuffer->AuLengthBits[i + 1];

  bitBuffer->AuReady--;

  return errInfo;
}

void DoCRCCheck(HANDLE_BIT_BUF hBitBuf, int check) {
  if (check)
    hBitBuf->crcFlag = 1;
  else
    hBitBuf->crcFlag = 0;
}

void InitCrcTab(HANDLE_BIT_BUF hBitStream) {
  int i;

  for (i = 0; i < CRCTAB_MAX_ENTRIES; i++)
    hBitStream->crcTab[i].id = IDC_INV;
}

unsigned int CrcSetBegin(HANDLE_BIT_BUF hBitStream, CRC_ID id) {
  int index = 0;
  if (hBitStream != NULL) {
    CRC_TAB *crcTab = hBitStream->crcTab;

    if (hBitStream->crcFlag != 0) {
      for (index = 0; index < CRCTAB_MAX_ENTRIES - 1; index++)
        if (crcTab[index].id == IDC_INV)
          break;
    }

    crcTab[index].id = id;
    crcTab[index].start = SetWritePointer(hBitStream, 0);
  }
  return (index);
}

void CrcSetEnd(HANDLE_BIT_BUF hBitStream, unsigned int index) {
  if (hBitStream != NULL) {
    hBitStream->crcTab[index].end = SetWritePointer(hBitStream, 0);
  }
}

unsigned short
crcCalc(unsigned short crcReg,
        unsigned short crcPoly,
        unsigned short crcMask,
        int noOfBits,
        HANDLE_BIT_BUF hBitstream) {
  int i;
  for (i = 0; i < noOfBits; i++) {
    unsigned short flag = (crcReg & crcMask) ? 1 : 0;
    flag ^= ReadBits(hBitstream, 1);
    crcReg <<= 1;
    if (flag)
      crcReg ^= crcPoly;
  }
  return (crcReg);
}
