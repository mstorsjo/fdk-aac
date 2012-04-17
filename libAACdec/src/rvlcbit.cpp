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

****************************************************************************/
/*!
  \file
  \brief  RVLC bitstream reading
  \author Robert Weidner
*/

#include "rvlcbit.h"


/*---------------------------------------------------------------------------------------------
     function:     rvlcReadBitFromBitstream

     description:  This function returns a bit from the bitstream according to read direction.
                   It is called very often, therefore it makes sense to inline it (runtime).
-----------------------------------------------------------------------------------------------
        input:     - bitstream
                   - pPosition
                   - readDirection
-----------------------------------------------------------------------------------------------
        return:    - bit from bitstream
-------------------------------------------------------------------------------------------- */

UCHAR rvlcReadBitFromBitstream (HANDLE_FDK_BITSTREAM  bs,
                                USHORT               *pPosition,
                                UCHAR                 readDirection)
{
  UINT   bit;
  INT    readBitOffset = *pPosition-FDKgetBitCnt(bs);

  if( readBitOffset ) {
    FDKpushBiDirectional(bs, readBitOffset);
  }

  if (readDirection == FWD) {
    bit = FDKreadBits(bs, 1);

    *pPosition += 1;
  } else {
    /* to be replaced with a brother function of FDKreadBits() */
    bit = FDKreadBits(bs, 1);
    FDKpushBack(bs, 2);

    *pPosition -= 1;
  }

  return (bit);
}

