/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (2009)
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
   contents/description: ADIF Transport Headers writing

******************************************************************************/

#include "tpenc_adif.h"

#include "tpenc_lib.h"
#include "tpenc_asc.h"



int adifWrite_EncodeHeader(ADIF_INFO *adif,
                                 HANDLE_FDK_BITSTREAM hBs,
                                 INT adif_buffer_fullness)
{
  /* ADIF/PCE/ADTS definitions */
  const char adifId[5]="ADIF";
  const int  copyRightIdPresent=0;
  const int  originalCopy=0;
  const int  home=0;

  int i;

  INT sampleRate = adif->samplingRate;
  INT totalBitRate = adif->bitRate;

  if (adif->headerWritten)
    return 0;

  /* Align inside PCE with respect to the first bit of the header */
  UINT alignAnchor = FDKgetValidBits(hBs);

  /* Signal variable bitrate if buffer fullnes exceeds 20 bit */
  adif->bVariableRate = ( adif_buffer_fullness >= (INT)(0x1<<20) ) ? 1 : 0;

  FDKwriteBits(hBs, adifId[0],8);
  FDKwriteBits(hBs, adifId[1],8);
  FDKwriteBits(hBs, adifId[2],8);
  FDKwriteBits(hBs, adifId[3],8);


  FDKwriteBits(hBs, copyRightIdPresent ? 1:0,1);

  if(copyRightIdPresent) {
    for(i=0;i<72;i++) {
      FDKwriteBits(hBs,0,1);
    }
  }
  FDKwriteBits(hBs, originalCopy ? 1:0,1);
  FDKwriteBits(hBs, home ? 1:0,1);
  FDKwriteBits(hBs, adif->bVariableRate?1:0, 1);
  FDKwriteBits(hBs, totalBitRate,23);

  /* we write only one PCE at the moment */
  FDKwriteBits(hBs, 0, 4);

  if(!adif->bVariableRate) {
    FDKwriteBits(hBs, adif_buffer_fullness, 20);
  }

  /* Write PCE */
  transportEnc_writePCE(hBs, adif->cm, sampleRate, adif->instanceTag, adif->profile, 0, 0, alignAnchor);

  return 0;
}

int adifWrite_GetHeaderBits(ADIF_INFO *adif)
{
  /* ADIF definitions */
  const int  copyRightIdPresent=0;

  if (adif->headerWritten)
    return 0;

  int bits = 0;

  bits += 8*4; /* ADIF ID */

  bits += 1; /* Copyright present */

  if (copyRightIdPresent)
    bits += 72;          /* Copyright ID */

  bits += 26;

  bits += 4; /* Number of PCE's */

  if(!adif->bVariableRate) {
    bits += 20;
  }

  /* write PCE */
  bits = transportEnc_GetPCEBits(adif->cm, 0, bits);

  return bits;
}

