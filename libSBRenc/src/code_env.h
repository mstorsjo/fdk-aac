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
  \brief  DPCM Envelope coding $Revision: 36867 $
*/

#ifndef __CODE_ENV_H
#define __CODE_ENV_H

#include "sbr_def.h"
#include "bit_sbr.h"
#include "fram_gen.h"

typedef struct
{
  INT offset;
  INT upDate;
  INT nSfb[2];
  SCHAR sfb_nrg_prev[MAX_FREQ_COEFFS];
  INT deltaTAcrossFrames;
  FIXP_DBL dF_edge_1stEnv;
  FIXP_DBL dF_edge_incr;
  INT dF_edge_incr_fac;


  INT codeBookScfLavTime;
  INT codeBookScfLavFreq;

  INT codeBookScfLavLevelTime;
  INT codeBookScfLavLevelFreq;
  INT codeBookScfLavBalanceTime;
  INT codeBookScfLavBalanceFreq;

  INT start_bits;
  INT start_bits_balance;


  const UCHAR *hufftableTimeL;
  const UCHAR *hufftableFreqL;

  const UCHAR *hufftableLevelTimeL;
  const UCHAR *hufftableBalanceTimeL;
  const UCHAR *hufftableLevelFreqL;
  const UCHAR *hufftableBalanceFreqL;
}
SBR_CODE_ENVELOPE;
typedef SBR_CODE_ENVELOPE *HANDLE_SBR_CODE_ENVELOPE;



void
FDKsbrEnc_codeEnvelope (SCHAR *sfb_nrg,
              const FREQ_RES *freq_res,
              SBR_CODE_ENVELOPE * h_sbrCodeEnvelope,
              INT *directionVec, INT coupling, INT nEnvelopes, INT channel,
              INT headerActive);

INT
FDKsbrEnc_InitSbrCodeEnvelope (HANDLE_SBR_CODE_ENVELOPE h_sbrCodeEnvelope,
                       INT *nSfb,
                       INT deltaTAcrossFrames,
                       FIXP_DBL dF_edge_1stEnv,
                       FIXP_DBL dF_edge_incr);

/* void deleteSbrCodeEnvelope (HANDLE_SBR_CODE_ENVELOPE h_sbrCodeEnvelope); */

INT
FDKsbrEnc_InitSbrHuffmanTables (struct SBR_ENV_DATA*      sbrEnvData,
                      HANDLE_SBR_CODE_ENVELOPE  henv,
                      HANDLE_SBR_CODE_ENVELOPE  hnoise,
                      AMP_RES                   amp_res);

#endif
