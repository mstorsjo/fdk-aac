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
   Author(s):   Christian Griebel
   Description: Dynamic range control (DRC) decoder tool for SBR

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef _SBRDEC_DRC_H_
#define _SBRDEC_DRC_H_

#include "sbrdecoder.h"


#define SBRDEC_MAX_DRC_CHANNELS  (6)
#define SBRDEC_MAX_DRC_BANDS     ( 16 )

typedef struct
{
  FIXP_DBL prevFact_mag[(64)];
  INT      prevFact_exp;

  FIXP_DBL currFact_mag[SBRDEC_MAX_DRC_BANDS];
  FIXP_DBL nextFact_mag[SBRDEC_MAX_DRC_BANDS];
  INT      currFact_exp;
  INT      nextFact_exp;

  UINT     numBandsCurr;
  UINT     numBandsNext;
  USHORT   bandTopCurr[SBRDEC_MAX_DRC_BANDS];
  USHORT   bandTopNext[SBRDEC_MAX_DRC_BANDS];

  SHORT    drcInterpolationSchemeCurr;
  SHORT    drcInterpolationSchemeNext;

  SHORT    enable;

  UCHAR    winSequenceCurr;
  UCHAR    winSequenceNext;

} SBRDEC_DRC_CHANNEL;

typedef SBRDEC_DRC_CHANNEL * HANDLE_SBR_DRC_CHANNEL;


void sbrDecoder_drcInitChannel (
    HANDLE_SBR_DRC_CHANNEL  hDrcData );

void sbrDecoder_drcUpdateChannel (
    HANDLE_SBR_DRC_CHANNEL  hDrcData );

void sbrDecoder_drcApplySlot (
    HANDLE_SBR_DRC_CHANNEL  hDrcData,
    FIXP_DBL *qmfRealSlot,
    FIXP_DBL *qmfImagSlot,
    int  col,
    int  numQmfSubSamples,
    int  maxShift );

void sbrDecoder_drcApply (
    HANDLE_SBR_DRC_CHANNEL  hDrcData,
    FIXP_DBL **QmfBufferReal,
    FIXP_DBL **QmfBufferImag,
    int  numQmfSubSamples,
    int *scaleFactor );


#endif  /* _SBRDEC_DRC_H_ */
