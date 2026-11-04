
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

#ifndef BIT_BUF_H
#define BIT_BUF_H

#include <stdio.h>
#include <assert.h>

#include "iisutillib.h"

#ifndef __cplusplus
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#endif

typedef int BIT_BUF_WORD;
typedef struct CRC_INFO *HANDLE_CRC_INFO;

typedef unsigned short bbWord;

#define MAX_AU_IN_BUFFER 200
#define CRCTAB_MAX_ENTRIES 30

typedef enum {
  IDC_SCE = 0,
  IDC_CPE = 1,
  IDC_CCE = 2,
  IDC_LFE = 3,
  IDC_2ICS = 4,
  IDC_HEAD = 5,
  IDC_PCE = 6,
  IDC_DSE = 7,
  IDC_INV = 8
} CRC_ID;

typedef struct {
  char date[20];
  char versionNo[20];
} Mp4bitbufLibInfo;

void Mp4bitbufGetLibInfo(Mp4bitbufLibInfo *libInfo);

typedef struct bitbuffer *HANDLE_BIT_BUF;

HANDLE_BIT_BUF CreateBitBuffer(int bitSize);

void DeleteBitBuffer(HANDLE_BIT_BUF hBitBuf);

void ResetBitBuffer(HANDLE_BIT_BUF hBitBuf);

int ResetReadPointer(HANDLE_BIT_BUF hBitBuf);

int GetBitBufSize(HANDLE_BIT_BUF hBitBuf);

int SetReadPointer(HANDLE_BIT_BUF hBitBuf, int delta);

void ByteAlignReadPointer(HANDLE_BIT_BUF hBitBuf);

int SetWritePointer(HANDLE_BIT_BUF hBitBuf, int delta);

int GetWritePointer(HANDLE_BIT_BUF hBitBuf);

int GetBitsAvail(HANDLE_BIT_BUF hBitBuf);

int WriteBits(HANDLE_BIT_BUF hBitBuf, BIT_BUF_WORD bValue, int nBits);

void WriteBytes(HANDLE_BIT_BUF hBitBuf, unsigned char const *source, int n);

BIT_BUF_WORD ReadBits(HANDLE_BIT_BUF hBitBuf, int nBits);

void ReadBytes(HANDLE_BIT_BUF hBitBuf, unsigned char *dst, int nBytes);

int ByteAlign(HANDLE_BIT_BUF hBitBuf);

void ngsCopyBits(HANDLE_BIT_BUF hBitBufWrite, HANDLE_BIT_BUF hBitBufRead, int nBits);

void UpdateBitstreamField(HANDLE_BIT_BUF hBitStream, int position, int value, int length);

HANDLE_ERROR_INFO IncAuReady(HANDLE_BIT_BUF bitBuffer, int AuLength);
HANDLE_ERROR_INFO DecAuReady(HANDLE_BIT_BUF bitBuffer, int *pNextAULen);
int GetAuReady(HANDLE_BIT_BUF bitBuffer);

void DoCRCCheck(HANDLE_BIT_BUF hBitBuf, int check);
void InitCrcTab(HANDLE_BIT_BUF hBitStream);
unsigned int CrcSetBegin(HANDLE_BIT_BUF hBitStream, CRC_ID id);
void CrcSetEnd(HANDLE_BIT_BUF hBitStream, unsigned int index);
unsigned short DoCrc(HANDLE_BIT_BUF hBitStream, HANDLE_CRC_INFO hCrcInfo);
unsigned short crcCalc(unsigned short crcReg, unsigned short crcPoly,
                       unsigned short crcMask, int noOfBits,
                       HANDLE_BIT_BUF hBitstream);

#endif
