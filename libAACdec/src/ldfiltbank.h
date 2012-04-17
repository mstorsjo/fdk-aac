/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (2006)
                                All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.

*
 *   filename: ldfiltbank.h
 *   project : MPEG-4 Audio Decoder
 *   contents/description: low delay filterbank interface
 *
 *   This software and/or program is protected by copyright law and
 *   international treaties. Any reproduction or distribution of this
 *   software and/or program, or any portion of it, may result in severe
 *   civil and criminal penalties, and will be prosecuted to the maximum
 *   extent possible under law.
 *
 * $Header: /home/cvs/amm/menc/mp4objdec/src/aac_eld/ldfiltbank.h,v 1.3 2006/11/07 22:21:27 haertlml Exp $
 *
\***************************************************************************/

#ifndef _LDFILTBANK_H
#define _LDFILTBANK_H

#include "common_fix.h"


int InvMdctTransformLowDelay_fdk (
        FIXP_DBL *mdctdata_m,
        const int mdctdata_e,
        INT_PCM *mdctOut,
        FIXP_DBL *fs_buffer,
        const int stride,
        const int frameLength
        );


#endif
