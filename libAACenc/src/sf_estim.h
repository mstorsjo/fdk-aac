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
   contents/description: Scale factor estimation

******************************************************************************/
#ifndef _SF_ESTIM_H
#define _SF_ESTIM_H

#include "common_fix.h"


#include "psy_const.h"
#include "qc_data.h"
#include "interface.h"

#define FORM_FAC_SHIFT  6


void
FDKaacEnc_CalcFormFactor(QC_OUT_CHANNEL   *qcOutChannel[(2)],
               PSY_OUT_CHANNEL  *psyOutChannel[(2)],
               const INT        nChannels);

void
FDKaacEnc_EstimateScaleFactors(PSY_OUT_CHANNEL *psyOutChannel[],
                     QC_OUT_CHANNEL* qcOutChannel[],
                     const int invQuant,
                     const int nChannels);



#endif
