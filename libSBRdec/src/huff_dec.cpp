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
  \brief  Huffman Decoder $Revision: 36841 $
*/

#include "huff_dec.h"

/***************************************************************************/
/*!
  \brief     Decodes one huffman code word

  Reads bits from the bitstream until a valid codeword is found.
  The table entries are interpreted either as index to the next entry
  or - if negative - as the codeword.

  \return    decoded value

  \author

****************************************************************************/
int
DecodeHuffmanCW (Huffman h,                /*!< pointer to huffman codebook table */
                 HANDLE_FDK_BITSTREAM hBs) /*!< Handle to Bitbuffer */
{
  SCHAR index = 0;
  int value, bit;

  while (index >= 0) {
    bit = FDKreadBits (hBs, 1);
    index = h[index][bit];
  }

  value = index+64; /* Add offset */


  return value;
}
