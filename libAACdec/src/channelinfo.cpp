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
   Description: individual channel stream info

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "channelinfo.h"
#include "aac_rom.h"
#include "aac_ram.h"
#include "FDK_bitstream.h"


AAC_DECODER_ERROR IcsReadMaxSfb (
        HANDLE_FDK_BITSTREAM bs,
        CIcsInfo *pIcsInfo,
        const SamplingRateInfo *pSamplingRateInfo
        )
{
  AAC_DECODER_ERROR ErrorStatus = AAC_DEC_OK;
  int nbits;

  if (IsLongBlock(pIcsInfo)) {
    nbits = 6;
    pIcsInfo->TotalSfBands = pSamplingRateInfo->NumberOfScaleFactorBands_Long;
  } else {
    nbits = 4;
    pIcsInfo->TotalSfBands = pSamplingRateInfo->NumberOfScaleFactorBands_Short;
  }
  pIcsInfo->MaxSfBands = (UCHAR) FDKreadBits(bs, nbits);

  if (pIcsInfo->MaxSfBands > pIcsInfo->TotalSfBands){
    ErrorStatus = AAC_DEC_PARSE_ERROR;
  }

  return ErrorStatus;
}



AAC_DECODER_ERROR IcsRead(HANDLE_FDK_BITSTREAM bs,
                          CIcsInfo *pIcsInfo,
                          const SamplingRateInfo* pSamplingRateInfo,
                          const UINT flags)
{
  AAC_DECODER_ERROR ErrorStatus = AAC_DEC_OK;

  pIcsInfo->Valid = 0;

  if (flags & AC_ELD){
    pIcsInfo->WindowSequence = OnlyLongSequence;
    pIcsInfo->WindowShape = 0;
  }
  else {
    if ( !(flags & (AC_USAC|AC_RSVD50)) ) {
      FDKreadBits(bs,1);
    }
    pIcsInfo->WindowSequence = (UCHAR) FDKreadBits(bs,2);
    pIcsInfo->WindowShape = (UCHAR) FDKreadBits(bs,1);
    if (flags & AC_LD) {
      if (pIcsInfo->WindowShape) {
        pIcsInfo->WindowShape = 2; /* select low overlap instead of KBD */
      }
    }
  }

  /* Sanity check */
  if ( (flags & (AC_ELD|AC_LD)) && pIcsInfo->WindowSequence != OnlyLongSequence) {
    pIcsInfo->WindowSequence = OnlyLongSequence;
    ErrorStatus = AAC_DEC_PARSE_ERROR;
    goto bail;
  }

  ErrorStatus = IcsReadMaxSfb(bs, pIcsInfo, pSamplingRateInfo);
  if (ErrorStatus != AAC_DEC_OK) {
    goto bail;
  }

  if (IsLongBlock(pIcsInfo))
  {
    if ( !(flags & (AC_ELD|AC_SCALABLE|AC_BSAC|AC_USAC|AC_RSVD50)) ) /* If not ELD nor Scalable nor BSAC nor USAC syntax then ... */
    {
      if ((UCHAR)FDKreadBits(bs,1) != 0 ) /* UCHAR PredictorDataPresent */
      {
        ErrorStatus = AAC_DEC_UNSUPPORTED_PREDICTION;
        goto bail;
      }
    }

    pIcsInfo->WindowGroups = 1;
    pIcsInfo->WindowGroupLength[0] = 1;
  }
  else
  {
    INT i;
    UINT mask;

    pIcsInfo->ScaleFactorGrouping = (UCHAR) FDKreadBits(bs,7);

    pIcsInfo->WindowGroups = 0 ;

    for (i=0; i < (8-1); i++)
    {
      mask = 1 << (6 - i);
      pIcsInfo->WindowGroupLength[i] = 1;

      if (pIcsInfo->ScaleFactorGrouping & mask)
      {
        pIcsInfo->WindowGroupLength[pIcsInfo->WindowGroups]++;
      }
      else
      {
        pIcsInfo->WindowGroups++;
      }
    }

    /* loop runs to i < 7 only */
    pIcsInfo->WindowGroupLength[8-1] = 1;
    pIcsInfo->WindowGroups++;
  }


bail:
  if (ErrorStatus == AAC_DEC_OK)
    pIcsInfo->Valid = 1;

  return ErrorStatus;
}


/*
  interleave codebooks the following way

    9 (84w) |  1 (51w)
   10 (82w) |  2 (39w)
  SCL (65w) |  4 (38w)
    3 (39w) |  5 (41w)
            |  6 (40w)
            |  7 (31w)
            |  8 (31w)
     (270w)     (271w)
*/


/*
  Table entries are sorted as following:
  | num_swb_long_window | sfbands_long | num_swb_short_window | sfbands_short |
*/
AAC_DECODER_ERROR getSamplingRateInfo(
        SamplingRateInfo *t,
        UINT samplesPerFrame,
        UINT samplingRateIndex,
        UINT samplingRate
        )
{
  int index = 0;


  t->samplingRateIndex = samplingRateIndex;
  t->samplingRate = samplingRate;

  switch (samplesPerFrame) {
  case 1024:
    index = 0;
    break;
  case 960:
    index = 1;
    break;
  case 512:
    index = 3;
    break;
  case 480:
    index = 4;
    break;

  default:
    return AAC_DEC_UNSUPPORTED_FORMAT;
  }

  t->ScaleFactorBands_Long = sfbOffsetTables[index][samplingRateIndex].sfbOffsetLong;
  t->ScaleFactorBands_Short = sfbOffsetTables[index][samplingRateIndex].sfbOffsetShort;
  t->NumberOfScaleFactorBands_Long = sfbOffsetTables[index][samplingRateIndex].numberOfSfbLong;
  t->NumberOfScaleFactorBands_Short = sfbOffsetTables[index][samplingRateIndex].numberOfSfbShort;

  if (t->ScaleFactorBands_Long == NULL || t->NumberOfScaleFactorBands_Long == 0) {
    return AAC_DEC_UNSUPPORTED_FORMAT;
  }

  FDK_ASSERT(t->ScaleFactorBands_Long[t->NumberOfScaleFactorBands_Long] == samplesPerFrame);
  FDK_ASSERT(t->ScaleFactorBands_Short == NULL || t->ScaleFactorBands_Short[t->NumberOfScaleFactorBands_Short]*8 == samplesPerFrame);

  return AAC_DEC_OK;
}
