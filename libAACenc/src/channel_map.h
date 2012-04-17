/*************************  Fast MPEG AAC Audio Encoder  **********************

                     (C) Copyright Fraunhofer IIS (2000)
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
   Initial author:       A. Groeschel
   contents/description: channel mapping functionality

******************************************************************************/

#ifndef _CHANNEL_MAP_H
#define _CHANNEL_MAP_H


#include "aacenc.h"
#include "psy_const.h"
#include "qc_data.h"

typedef struct {
  CHANNEL_MODE encMode;
  INT nChannels;
  INT nChannelsEff;
  INT nElements;
} CHANNEL_MODE_CONFIG_TAB;


/* Element mode */
typedef enum {
  EL_MODE_INVALID = 0,
  EL_MODE_MONO,
  EL_MODE_STEREO
} ELEMENT_MODE;


AAC_ENCODER_ERROR FDKaacEnc_DetermineEncoderMode(CHANNEL_MODE* mode,
                                                 INT nChannels);

AAC_ENCODER_ERROR FDKaacEnc_InitChannelMapping(CHANNEL_MODE mode,
                                               CHANNEL_ORDER co,
                                               CHANNEL_MAPPING* chMap);

AAC_ENCODER_ERROR FDKaacEnc_InitElementBits(QC_STATE *hQC,
                                            CHANNEL_MAPPING *cm,
                                            INT bitrateTot,
                                            INT averageBitsTot,
                                            INT maxChannelBits);

ELEMENT_MODE FDKaacEnc_GetMonoStereoMode(const CHANNEL_MODE mode);

const CHANNEL_MODE_CONFIG_TAB* FDKaacEnc_GetChannelModeConfiguration(const CHANNEL_MODE mode);

//void FDKaacEnc_CloseChannelMapping (CHANNEL_MAPPING** phchMap);

#endif /* CHANNEL_MAP_H */
