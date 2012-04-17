/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (1999)
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
   Initial author:       M.Werner
   contents/description: Interface psychoaccoustic/quantizer

******************************************************************************/
#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "common_fix.h"

#include "psy_data.h"
#include "aacenc_tns.h"

enum
{
  MS_NONE = 0,
  MS_SOME = 1,
  MS_ALL  = 2
};

enum
{
  MS_ON = 1
};

struct TOOLSINFO {
  INT msDigest;     /* 0 = no MS; 1 = some MS, 2 = all MS */
  INT msMask[MAX_GROUPED_SFB];
};


typedef struct  {
  INT       sfbCnt;
  INT       sfbPerGroup;
  INT       maxSfbPerGroup;
  INT       lastWindowSequence;
  INT       windowShape;
  INT       groupingMask;
  INT       sfbOffsets[MAX_GROUPED_SFB+1];

  INT       mdctScale;                      /* number of transform shifts */
  INT       groupLen[MAX_NO_OF_GROUPS];

  TNS_INFO  tnsInfo;
  INT       noiseNrg[MAX_GROUPED_SFB];
  INT       isBook[MAX_GROUPED_SFB];
  INT       isScale[MAX_GROUPED_SFB];

  /* memory located in QC_OUT_CHANNEL */
  FIXP_DBL  *mdctSpectrum;
  FIXP_DBL  *sfbEnergy;
  FIXP_DBL  *sfbSpreadEnergy;
  FIXP_DBL  *sfbThresholdLdData;
  FIXP_DBL  *sfbMinSnrLdData;
  FIXP_DBL  *sfbEnergyLdData;


 }PSY_OUT_CHANNEL;

typedef struct {

  /* information specific to each channel */
  PSY_OUT_CHANNEL* psyOutChannel[(2)];

  /* information shared by both channels  */
  INT       commonWindow;
  struct TOOLSINFO toolsInfo;

} PSY_OUT_ELEMENT;

typedef struct {

  PSY_OUT_ELEMENT* psyOutElement[(6)];
  PSY_OUT_CHANNEL* pPsyOutChannels[(6)];

}PSY_OUT;

#endif /* _INTERFACE_H */
