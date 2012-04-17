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
   contents/description: Threshold compensation

******************************************************************************/
#ifndef __ADJ_THR_H
#define __ADJ_THR_H


#include "adj_thr_data.h"
#include "qc_data.h"
#include "line_pe.h"
#include "interface.h"



void FDKaacEnc_peCalculation(PE_DATA *peData,
                             PSY_OUT_CHANNEL* psyOutChannel[(2)],
                             QC_OUT_CHANNEL* qcOutChannel[(2)],
                             struct TOOLSINFO *toolsInfo,
                             ATS_ELEMENT* adjThrStateElement,
                             const INT nChannels);

INT  FDKaacEnc_AdjThrNew(ADJ_THR_STATE** phAdjThr,
                         INT             nElements);

void FDKaacEnc_AdjThrInit(ADJ_THR_STATE *hAdjThr,
                const INT peMean,
                ELEMENT_BITS* elBits[(6)],
                INT nElements,
                FIXP_DBL vbrQualFactor);


void FDKaacEnc_DistributeBits(ADJ_THR_STATE *adjThrState,
    ATS_ELEMENT       *AdjThrStateElement,
    PSY_OUT_CHANNEL   *psyOutChannel[(2)],
    PE_DATA           *peData,
    INT               *grantedPe,
    INT               *grantedPeCorr,
    const INT         nChannels,
    const INT         commonWindow,
    const INT         avgBits,
    const INT         bitresBits,
    const INT         maxBitresBits,
    const FIXP_DBL    maxBitFac,
    const INT         bitDistributenMode);

void FDKaacEnc_AdjustThresholds(ATS_ELEMENT* AdjThrStateElement[(6)],
    QC_OUT_ELEMENT*   qcElement[(6)],
    QC_OUT*           qcOut,
    PSY_OUT_ELEMENT*  psyOutElement[(6)],
    INT               CBRbitrateMode,
    CHANNEL_MAPPING*  cm);

void FDKaacEnc_AdjThrClose(ADJ_THR_STATE** hAdjThr);

#endif
