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
   Initial author:       M. Werner
   contents/description: MDCT Transform

******************************************************************************/
#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "common_fix.h"

#define WTS0 1
#define WTS1 0
#define WTS2 -2

/**
 * \brief: Performe MDCT transform of time domain data.
 * \param timeData pointer to time domain input signal.
 * \param mdctData pointer to store frequency domain output data.
 * \param blockType index indicating the type of block. Either
 *        LONG_WINDOW, START_WINDOW, SHORT_WINDOW or STOP_WINDOW.
 * \param windowShape index indicating the window slope type to be used.
 *        Values allowed are either SINE_WINDOW or KBD_WINDOW.
 * \param frameLength length of the block. Either 1024 or 960.
 * \param mdctData_e pointer to an INT where the exponent of the frequency
 *        domain output data is stored into.
 * \return 0 in case of success, non-zero in case of error (inconsistent parameters).
 */
INT FDKaacEnc_Transform_Real (const INT_PCM *timeData,
                              FIXP_DBL *RESTRICT mdctData,
                              const INT blockType,
                              const INT windowShape,
                              INT *prevWindowShape,
                              const INT frameLength,
                              INT *mdctData_e,
                              INT filterType
                             ,FIXP_DBL * RESTRICT overlapAddBuffer
                             );
#endif
