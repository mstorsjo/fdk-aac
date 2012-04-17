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
   Description: pulse data tool

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "pulsedata.h"


#include "channelinfo.h"


INT CPulseData_Read(
        HANDLE_FDK_BITSTREAM bs,
        CPulseData *const PulseData,
        const SHORT *sfb_startlines,
        const void *pIcsInfo,
        const SHORT frame_length
        )
{
  int i, k=0;
  const UINT MaxSfBands = GetScaleFactorBandsTransmitted((CIcsInfo*)pIcsInfo);

  /* reset pulse data flag */
  PulseData->PulseDataPresent = 0;

  if ((PulseData->PulseDataPresent = (UCHAR) FDKreadBit(bs)) != 0) {
    if (!IsLongBlock((CIcsInfo*)pIcsInfo)) {
      return AAC_DEC_DECODE_FRAME_ERROR;
    }

    PulseData->NumberPulse = (UCHAR) FDKreadBits(bs,2);
    PulseData->PulseStartBand = (UCHAR) FDKreadBits(bs,6);

    if (PulseData->PulseStartBand >= MaxSfBands) {
      return AAC_DEC_DECODE_FRAME_ERROR;
    }

    k = sfb_startlines[PulseData->PulseStartBand];

    for (i=0; i<=PulseData->NumberPulse; i++) {
      PulseData->PulseOffset[i] = (UCHAR) FDKreadBits(bs,5);
      PulseData->PulseAmp[i] = (UCHAR) FDKreadBits(bs,4);
      k += PulseData->PulseOffset[i];
    }

    if (k >= frame_length) {
        return AAC_DEC_DECODE_FRAME_ERROR;
    }
  }


  return 0;
}

void CPulseData_Apply(CPulseData *PulseData,                /*!< pointer to pulse data side info */
                      const short *pScaleFactorBandOffsets, /*!< pointer to scalefactor band offsets */
                      FIXP_DBL *coef)                     /*!< pointer to spectrum */
{
  int i,k;

  if (PulseData->PulseDataPresent)
  {
    k = pScaleFactorBandOffsets[PulseData->PulseStartBand];

    for (i=0; i<=PulseData->NumberPulse; i++)
    {
      k += PulseData->PulseOffset[i];
      if (coef [k] > (FIXP_DBL)0) coef[k] += (FIXP_DBL)(int)PulseData->PulseAmp[i];
      else                          coef[k] -= (FIXP_DBL)(int)PulseData->PulseAmp[i];
    }
  }
}

