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
  \brief  CRC checking routines $Revision: 36841 $
*/
#ifndef __SBR_CRC_H
#define __SBR_CRC_H

#include "sbrdecoder.h"

#include "FDK_bitstream.h"

/* some useful crc polynoms:

crc5: x^5+x^4+x^2+x^1+1
crc6: x^6+x^5+x^3+x^2+x+1
crc7: x^7+x^6+x^2+1
crc8: x^8+x^2+x+x+1
*/

/* default SBR CRC */  /* G(x) = x^10 + x^9 + x^5 + x^4 + x + 1 */
#define SBR_CRC_POLY                  0x0233
#define SBR_CRC_MASK                  0x0200
#define SBR_CRC_START                 0x0000
#define SBR_CRC_RANGE                 0x03FF

typedef struct
{
  USHORT crcState;
  USHORT crcMask;
  USHORT crcPoly;
}
CRC_BUFFER;

typedef CRC_BUFFER *HANDLE_CRC;

int SbrCrcCheck (HANDLE_FDK_BITSTREAM hBitBuf,
                 LONG                 NrCrcBits);


#endif
