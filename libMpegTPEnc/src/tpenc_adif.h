/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (2009)
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
   contents/description: Transport Headers support

******************************************************************************/

#ifndef TPENC_ADIF_H
#define TPENC_ADIF_H

#include "machine_type.h"
#include "FDK_bitstream.h"

#include "tp_data.h"

typedef struct {
  CHANNEL_MODE cm;
  INT samplingRate;
  INT bitRate;
  int profile;
  int bVariableRate;
  int instanceTag;
  int headerWritten;
} ADIF_INFO;

/**
 * \brief encodes ADIF Header
 *
 * \param adif pointer to ADIF_INFO structure
 * \param hBitStream handle of bitstream, where the ADIF header is written into
 * \param adif_buffer_fullness buffer fullness value for the ADIF header
 *
 * \return 0 on success
 */
int adifWrite_EncodeHeader(
        ADIF_INFO           *adif,
        HANDLE_FDK_BITSTREAM hBitStream,
        INT                  adif_buffer_fullness
        );

/**
 * \brief Get bit demand of a ADIF header
 *
 * \param adif pointer to ADIF_INFO structure
 *
 * \return amount of bits required to write the ADIF header according to the data
 *         contained in the adif parameter
 */
int adifWrite_GetHeaderBits( ADIF_INFO *adif );

#endif /* TPENC_ADIF_H */

