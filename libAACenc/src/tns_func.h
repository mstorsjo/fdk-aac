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
   Initial author:       Alex Goeschel
   contents/description: Temporal noise shaping

******************************************************************************/
#ifndef _TNS_FUNC_H
#define _TNS_FUNC_H

#include "common_fix.h"

#include "psy_configuration.h"

AAC_ENCODER_ERROR FDKaacEnc_InitTnsConfiguration(INT bitrate,
                         INT samplerate,
                         INT channels,
                         INT blocktype,
                         INT granuleLength,
                         INT ldSbrPresent,
                         TNS_CONFIG *tnsConfig,
                         PSY_CONFIGURATION *psyConfig,
                         INT active,
                         INT useTnsPeak );

INT FDKaacEnc_TnsDetect(
              TNS_DATA *tnsData,
              const TNS_CONFIG *tC,
              TNS_INFO* tnsInfo,
              INT sfbCnt,
              FIXP_DBL *spectrum,
              INT subBlockNumber,
              INT blockType
              );



void FDKaacEnc_TnsSync(
             TNS_DATA *tnsDataDest,
             const TNS_DATA *tnsDataSrc,
             TNS_INFO *tnsInfoDest,
             TNS_INFO *tnsInfoSrc,
             const INT blockTypeDest,
             const INT blockTypeSrc,
             const TNS_CONFIG *tC
             );

INT FDKaacEnc_TnsEncode(
              TNS_INFO* tnsInfo,
              TNS_DATA* tnsData,
              const INT numOfSfb,
              const TNS_CONFIG *tC,
              const INT lowPassLine,
              FIXP_DBL* spectrum,
              const INT subBlockNumber,
              const INT blockType
              );



#endif /* _TNS_FUNC_H */
