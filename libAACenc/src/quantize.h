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
   contents/description: Quantization

******************************************************************************/
#ifndef _QUANTIZE_H_
#define _QUANTIZE_H_

#include "common_fix.h"

/* quantizing */

#define MAX_QUANT 8191

void FDKaacEnc_QuantizeSpectrum(INT sfbCnt,
                      INT maxSfbPerGroup,
                      INT sfbPerGroup,
                      INT *sfbOffset, FIXP_DBL *mdctSpectrum,
                      INT globalGain, INT *scalefactors,
                      SHORT *quantizedSpectrum);

FIXP_DBL FDKaacEnc_calcSfbDist(FIXP_DBL *mdctSpectrum,
                     SHORT *quantSpectrum,
                     INT noOfLines,
                     INT gain);

void FDKaacEnc_calcSfbQuantEnergyAndDist(FIXP_DBL *mdctSpectrum,
                               SHORT   *quantSpectrum,
                               INT    noOfLines,
                               INT    gain,
                               FIXP_DBL *en,
                               FIXP_DBL *dist);

#endif /* _QUANTIZE_H_ */
