/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (2010)
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

   Initial author:       A. Horndasch (code originally from lwr and rtb) / Josef Höpfl (FDK)
   contents/description: intensity stereo prototype

******************************************************************************/

#ifndef _INTENSITY_H
#define _INTENSITY_H

#include "aacenc_pns.h"


void FDKaacEnc_IntensityStereoProcessing(
        FIXP_DBL                  *sfbEnergyLeft,
        FIXP_DBL                  *sfbEnergyRight,
        FIXP_DBL                  *mdctSpectrumLeft,
        FIXP_DBL                  *mdctSpectrumRight,
        FIXP_DBL                  *sfbThresholdLeft,
        FIXP_DBL                  *sfbThresholdRight,
        FIXP_DBL                  *sfbThresholdLdDataRight,
        FIXP_DBL                  *sfbSpreadEnLeft,
        FIXP_DBL                  *sfbSpreadEnRight,
        FIXP_DBL                  *sfbEnergyLdDataLeft,
        FIXP_DBL                  *sfbEnergyLdDataRight,
        INT                       *msDigest,
        INT                       *msMask,
        const INT                  sfbCnt,
        const INT                  sfbPerGroup,
        const INT                  maxSfbPerGroup,
        const INT                 *sfbOffset,
        const INT                  allowIS,
        INT                       *isBook,
        INT                       *isScale,
        PNS_DATA         *RESTRICT pnsData[2]
        );

#endif /* _INTENSITY_H */

