/*****************************  MPEG-4 AAC Decoder  ***************************

                      (C) Copyright Fraunhofer IIS 2000-2008
                               All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.


   $Id$
   Author(s):   Robert Weidner (DSP Solutions)
   Description: HCR Decoder: Bitstream reading

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

*******************************************************************************/

#include "aacdec_hcr_bit.h"


/*---------------------------------------------------------------------------------------------
        description:  This function toggles the read direction.
-----------------------------------------------------------------------------------------------
        input:        current read direction
-----------------------------------------------------------------------------------------------
        return:       new read direction
-------------------------------------------------------------------------------------------- */
UCHAR ToggleReadDirection(UCHAR readDirection)
{
  if ( readDirection == FROM_LEFT_TO_RIGHT ) {
    return FROM_RIGHT_TO_LEFT;
  }
  else {
    return FROM_LEFT_TO_RIGHT;
  }
}


/*---------------------------------------------------------------------------------------------
     description:   This function returns a bit from the bitstream according to read direction.
                    It is called very often, therefore it makes sense to inline it (runtime).
-----------------------------------------------------------------------------------------------
        input:    - handle to FDK bitstream
                  - reference value marking start of bitfield
                  - pLeftStartOfSegment
                  - pRightStartOfSegment
                  - readDirection
-----------------------------------------------------------------------------------------------
        return:   - bit from bitstream
-------------------------------------------------------------------------------------------- */
UINT HcrGetABitFromBitstream(HANDLE_FDK_BITSTREAM  bs,
                             USHORT               *pLeftStartOfSegment,
                             USHORT               *pRightStartOfSegment,
                             UCHAR                 readDirection)
{
  UINT   bit;
  INT    readBitOffset;

  if (readDirection == FROM_LEFT_TO_RIGHT) {
    readBitOffset = *pLeftStartOfSegment-FDKgetBitCnt(bs);
    if( readBitOffset ) {
      FDKpushBiDirectional(bs, readBitOffset);
    }

    bit = FDKreadBits(bs, 1);

    *pLeftStartOfSegment += 1;
  }
  else {
    readBitOffset = *pRightStartOfSegment-FDKgetBitCnt(bs);
    if( readBitOffset ) {
      FDKpushBiDirectional(bs, readBitOffset);
    }

    /* to be replaced with a brother function of FDKreadBits() */
    bit = FDKreadBits(bs, 1);
    FDKpushBack(bs, 2);

    *pRightStartOfSegment -= 1;
  }


#if ERROR_GENERATOR_BIT_STREAM_HCR
  static int a;
  if ((++a % MODULO_DIVISOR_HCR) == 0) {
    bit = (bit == 0) ? 1 : 0;
  }
#endif

  return (bit);
}

