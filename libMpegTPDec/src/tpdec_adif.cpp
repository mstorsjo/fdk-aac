/*****************************  MPEG-4 AAC Decoder  **************************

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


   $Id$
   Author(s):   Josef Hoepfl
   Description: ADIF reader

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "tpdec_adif.h"


#include "FDK_bitstream.h"
#include "genericStds.h"

TRANSPORTDEC_ERROR adifRead_DecodeHeader(
        CAdifHeader          *pAdifHeader,
        CProgramConfig       *pPce,
        HANDLE_FDK_BITSTREAM  bs
        )
{
  int i;
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;
  UINT startAnchor = FDKgetValidBits(bs);

  if ((INT)startAnchor < MIN_ADIF_HEADERLENGTH) {
    return (TRANSPORTDEC_NOT_ENOUGH_BITS);
  }

  if (FDKreadBits(bs,8) != 'A') {
    return (TRANSPORTDEC_SYNC_ERROR);
  }
  if (FDKreadBits(bs,8) != 'D') {
    return (TRANSPORTDEC_SYNC_ERROR);
  }
  if (FDKreadBits(bs,8) != 'I') {
    return (TRANSPORTDEC_SYNC_ERROR);
  }
  if (FDKreadBits(bs,8) != 'F') {
    return (TRANSPORTDEC_SYNC_ERROR);
  }

  if ( (pAdifHeader->CopyrightIdPresent = FDKreadBits(bs,1)) != 0 )
    FDKpushBiDirectional(bs,72);  /* CopyrightId */


  pAdifHeader->OriginalCopy = FDKreadBits(bs,1);
  pAdifHeader->Home = FDKreadBits(bs,1);
  pAdifHeader->BitstreamType = FDKreadBits(bs,1);

  /* pAdifHeader->BitRate = FDKreadBits(bs, 23); */
  pAdifHeader->BitRate = FDKreadBits(bs,16);
  pAdifHeader->BitRate <<= 7;
  pAdifHeader->BitRate |= FDKreadBits(bs,7);

  pAdifHeader->NumProgramConfigElements = FDKreadBits(bs,4) + 1;

  if (pAdifHeader->BitstreamType == 0) {
    FDKpushBiDirectional(bs,20);  /* adif_buffer_fullness */
  }

  /* Parse all PCEs but keep only one */
  for (i=0; i < pAdifHeader->NumProgramConfigElements; i++)
  {
    CProgramConfig_Read(pPce, bs, startAnchor);
  }

  FDKbyteAlign(bs, startAnchor);

  return (ErrorStatus);
}



