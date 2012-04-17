/*************************  Fast MPEG AAC Audio Encoder  **********************

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
   Initial author:       M. Schug / A. Groeschel
   contents/description: bandwidth expert

******************************************************************************/

#ifndef _BANDWIDTH_H
#define _BANDWIDTH_H


#include "qc_data.h"

AAC_ENCODER_ERROR FDKaacEnc_DetermineBandWidth(INT* bandWidth,
                                               INT proposedBandwidth,
                                               INT bitrate,
                                               AACENC_BITRATE_MODE bitrateMode,
                                               INT sampleRate,
                                               INT frameLength,
                                               CHANNEL_MAPPING* cm,
                                               CHANNEL_MODE encoderMode);

#endif /* BANDWIDTH_H */
