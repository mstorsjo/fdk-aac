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
   Author(s):   Josef Hoepfl
   Description: pulse data tool

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef PULSEDATA_H
#define PULSEDATA_H



#include "common_fix.h"
#include "FDK_bitstream.h"

#define N_MAX_LINES 4

typedef struct
{
  UCHAR PulseDataPresent;
  UCHAR NumberPulse;
  UCHAR PulseStartBand;
  UCHAR PulseOffset[N_MAX_LINES];
  UCHAR PulseAmp[N_MAX_LINES];
} CPulseData;

/**
 * \brief Read pulse data from bitstream
 *
 * The function reads the elements for pulse data from
 * the bitstream.
 *
 * \param bs bit stream handle data source.
 * \param PulseData pointer to a CPulseData were the decoded data is stored into.
 * \param MaxSfBands max number of scale factor bands.
 * \return 0 on success, != 0 on parse error.
 */
INT CPulseData_Read(
        const HANDLE_FDK_BITSTREAM bs,
        CPulseData *const PulseData,
        const SHORT *sfb_startlines,
        const void *pIcsInfo,
        const SHORT frame_length
        );

/**
 * \brief Apply pulse data to spectral lines
 *
 * The function applies the pulse data to the
 * specified spectral lines.
 *
 * \param PulseData pointer to the previously decoded pulse data.
 * \param pScaleFactorBandOffsets scale factor band line offset table.
 * \param coef pointer to the spectral data were pulse data should be applied to.
 * \return  none
*/
void CPulseData_Apply(CPulseData *PulseData,
                      const short *pScaleFactorBandOffsets,
                      FIXP_DBL *coef);


#endif /* #ifndef PULSEDATA_H */
