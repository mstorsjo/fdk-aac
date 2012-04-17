/****************************************************************************

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


   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

  $Id$
****************************************************************************/
/*!
  \file
  \brief   Frequency scale prototypes $Revision: 36841 $
*/
#ifndef __FREQ_SCA_H
#define __FREQ_SCA_H

#include "sbrdecoder.h"
#include "env_extr.h"

int
sbrdecUpdateFreqScale(UCHAR * v_k_master,
                      UCHAR *numMaster,
                      HANDLE_SBR_HEADER_DATA headerData);

void sbrdecDownSampleLoRes(UCHAR *v_result, UCHAR num_result,
                           UCHAR *freqBandTableRef, UCHAR num_Ref);

void shellsort(UCHAR *in, UCHAR n);

SBR_ERROR
resetFreqBandTables(HANDLE_SBR_HEADER_DATA hHeaderData, const UINT flags);

#endif
