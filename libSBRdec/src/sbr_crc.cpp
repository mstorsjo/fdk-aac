/****************************************************************************

                     (C) Copyright Fraunhofer IIS (2004)
                               All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.


   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

 $Id$

*******************************************************************************/
/*!
  \file
  \brief  CRC check coutines $Revision: 36841 $
*/

#include "sbr_crc.h"

#include "FDK_bitstream.h"
#include "transcendent.h"

#define MAXCRCSTEP    16
#define MAXCRCSTEP_LD  4

/*!
  \brief     crc calculation
*/
static ULONG
calcCRC (HANDLE_CRC hCrcBuf, ULONG bValue, int nBits)
{
  int i;
  ULONG bMask = (1UL << (nBits - 1));

  for (i = 0; i < nBits; i++, bMask >>= 1) {
    USHORT flag = (hCrcBuf->crcState & hCrcBuf->crcMask) ? 1 : 0;
    USHORT flag1 = (bMask & bValue) ? 1 : 0;

    flag ^= flag1;
    hCrcBuf->crcState <<= 1;
    if (flag)
      hCrcBuf->crcState ^= hCrcBuf->crcPoly;
  }

  return (hCrcBuf->crcState);
}


/*!
  \brief     crc
*/
static int
getCrc (HANDLE_FDK_BITSTREAM hBs, ULONG NrBits)
{
  int i;
  CRC_BUFFER CrcBuf;

  CrcBuf.crcState = SBR_CRC_START;
  CrcBuf.crcPoly  = SBR_CRC_POLY;
  CrcBuf.crcMask  = SBR_CRC_MASK;

  int CrcStep = NrBits>>MAXCRCSTEP_LD;

  int CrcNrBitsRest = (NrBits - CrcStep * MAXCRCSTEP);
  ULONG bValue;

  for (i = 0; i < CrcStep; i++) {
    bValue = FDKreadBits (hBs, MAXCRCSTEP);
    calcCRC (&CrcBuf, bValue, MAXCRCSTEP);
  }

  bValue = FDKreadBits (hBs, CrcNrBitsRest);
  calcCRC (&CrcBuf, bValue, CrcNrBitsRest);

  return (CrcBuf.crcState & SBR_CRC_RANGE);

}


/*!
  \brief   crc interface
  \return  1: CRC OK, 0: CRC check failure
*/
int
SbrCrcCheck (HANDLE_FDK_BITSTREAM hBs, /*!< handle to bit-buffer  */
             LONG NrBits)              /*!< max. CRC length       */
{
  int crcResult = 1;
  ULONG NrCrcBits;
  ULONG crcCheckResult;
  LONG NrBitsAvailable;
  ULONG crcCheckSum;

  crcCheckSum = FDKreadBits (hBs, 10);

  NrBitsAvailable = FDKgetValidBits(hBs);
  if (NrBitsAvailable <= 0){
    return 0;
  }

  NrCrcBits = fixMin ((INT)NrBits, (INT)NrBitsAvailable);

  crcCheckResult = getCrc (hBs, NrCrcBits);
  FDKpushBack(hBs, (NrBitsAvailable - FDKgetValidBits(hBs)) );


  if (crcCheckResult != crcCheckSum) {
    crcResult = 0;
  }

  return (crcResult);
}
