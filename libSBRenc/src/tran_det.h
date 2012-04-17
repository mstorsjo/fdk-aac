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

*******************************************************************************/
/*!
  \file
  \brief  Transient detector prototypes $Revision: 36847 $
*/
#ifndef __TRAN_DET_H
#define __TRAN_DET_H

#include "sbr_encoder.h"
#include "sbr_def.h"

typedef struct
{
  FIXP_DBL  transients[QMF_MAX_TIME_SLOTS+(QMF_MAX_TIME_SLOTS/2)];
  FIXP_DBL  thresholds[QMF_CHANNELS];
  FIXP_DBL  tran_thr;              /* Master threshold for transient signals */
  FIXP_DBL  split_thr;             /* Threshold for splitting FIXFIX-frames into 2 env */
  FIXP_DBL  prevLowBandEnergy;     /* Energy of low band */
  FIXP_DBL  prevHighBandEnergy;    /* Energy of high band */
  INT    tran_fc;                  /* Number of lowband subbands to discard  */
  INT    no_cols;
  INT    no_rows;
  INT    mode;

  int    frameShift;
  int    tran_off;                 /* Offset for reading energy values. */
}
SBR_TRANSIENT_DETECTOR;


typedef SBR_TRANSIENT_DETECTOR *HANDLE_SBR_TRANSIENT_DETECTOR;

void
FDKsbrEnc_transientDetect(HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,
                          FIXP_DBL **Energies,
                          INT *scaleEnergies,
                          UCHAR *tran_vector,
                          int YBufferWriteOffset,
                          int YBufferSzShift,
                          int timeStep,
                          int frameMiddleBorder);

int
FDKsbrEnc_InitSbrTransientDetector (HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,
                            INT   frameSize,
                            INT   sampleFreq,
                            sbrConfigurationPtr params,
                            int   tran_fc,
                            int   no_cols,
                            int   no_rows,
                            int   YBufferWriteOffset,
                            int   YBufferSzShift,
                            int   frameShift,
                            int   tran_off);

void
FDKsbrEnc_frameSplitter(FIXP_DBL **Energies,
                        INT *scaleEnergies,
                        HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,
                        UCHAR *freqBandTable,
                        UCHAR *tran_vector,
                        int YBufferWriteOffset,
                        int YBufferSzShift,
                        int nSfb,
                        int timeStep,
                        int no_cols);

#endif
