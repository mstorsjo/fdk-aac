/****************************************************************************

                       (C) copyright Fraunhofer-IIS (2004)
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

*******************************************************************************/
/*!
  \file
  \brief  frequency scale prototypes $Revision: 36867 $
*/
#ifndef __FREQ_SCA2_H
#define __FREQ_SCA2_H

#include "sbr_encoder.h"
#include "sbr_def.h"

#define MAX_OCTAVE        29
#define MAX_SECOND_REGION 50


INT
FDKsbrEnc_UpdateFreqScale(UCHAR *v_k_master, INT *h_num_bands,
                const INT k0, const INT k2,
                const INT freq_scale,
                const INT alter_scale);

INT
FDKsbrEnc_UpdateHiRes(UCHAR *h_hires,
            INT *num_hires,
            UCHAR *v_k_master,
            INT num_master ,
            INT *xover_band,
            SR_MODE drOrSr,
            INT noQMFChannels);

void  FDKsbrEnc_UpdateLoRes(UCHAR * v_lores,
                  INT *num_lores,
                  UCHAR * v_hires,
                  INT num_hires);

INT
FDKsbrEnc_FindStartAndStopBand(const INT samplingFreq,
                     const INT noChannels,
                     const INT startFreq,
                     const INT stop_freq,
                     const SR_MODE sampleRateMode,
                     INT *k0,
                     INT *k2);

INT FDKsbrEnc_getSbrStartFreqRAW (INT startFreq, INT QMFbands, INT fs );
INT FDKsbrEnc_getSbrStopFreqRAW  (INT stopFreq, INT QMFbands, INT fs);
#endif
