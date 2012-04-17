/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (2001)
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

   Initial author:       M. Lohwasser
   contents/description: pns_func.h

******************************************************************************/

#ifndef _PNS_FUNC_H
#define _PNS_FUNC_H

#include "common_fix.h"

#include "aacenc_pns.h"
#include "psy_data.h"



AAC_ENCODER_ERROR FDKaacEnc_InitPnsConfiguration(PNS_CONFIG *pnsConf,
                                                 INT        bitRate,
                                                 INT        sampleRate,
                                                 INT        usePns,
                                                 INT        sfbCnt,
                                                 const INT  *sfbOffset,
                                                 const INT  numChan,
                                                 const INT  isLC );

void FDKaacEnc_PnsDetect( PNS_CONFIG  *pnsConf,
                PNS_DATA    *pnsData,
                const INT   lastWindowSequence,
                const INT   sfbActive,
                const INT   maxSfbPerGroup,
                FIXP_DBL    *sfbThresholdLdData,
                const INT   *sfbOffset,
                FIXP_DBL    *mdctSpectrum,
                INT         *sfbMaxScaleSpec,
                FIXP_SGL    *sfbtonality,
                int         tnsOrder,
                INT         tnsPredictionGain,
                INT         tnsActive,
                FIXP_DBL    *sfbEnergyLdData,
                INT         *noiseNrg );

void FDKaacEnc_CodePnsChannel( const INT     sfbActive,
                     PNS_CONFIG    *pnsConf,
                     INT           *pnsFlag,
                     FIXP_DBL      *sfbEnergy,
                     INT           *noiseNrg,
                     FIXP_DBL      *sfbThreshold );

void FDKaacEnc_PreProcessPnsChannelPair( const INT  sfbActive,
                               FIXP_DBL   *sfbEnergyLeft,
                               FIXP_DBL   *sfbEnergyRight,
                               FIXP_DBL   *sfbEnergyLeftLD,
                               FIXP_DBL   *sfbEnergyRightLD,
                               FIXP_DBL   *sfbEnergyMid,
                               PNS_CONFIG *pnsConfLeft,
                               PNS_DATA   *pnsDataLeft,
                               PNS_DATA   *pnsDataRight );

void FDKaacEnc_PostProcessPnsChannelPair( const INT   sfbActive,
                                PNS_CONFIG  *pnsConf,
                                PNS_DATA    *pnsDataLeft,
                                PNS_DATA    *pnsDataRight,
                                INT         *msMask,
                                INT         *msDigest );

#endif /* _PNS_FUNC_H */
