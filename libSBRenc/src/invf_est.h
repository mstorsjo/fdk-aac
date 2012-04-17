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
  \brief  Inverse Filtering detection prototypes $Revision: 36847 $
*/
#ifndef _INV_FILT_DET_H
#define _INV_FILT_DET_H

#include "sbr_encoder.h"
#include "sbr_def.h"

#define INVF_SMOOTHING_LENGTH 2

typedef struct
{
  const FIXP_DBL *quantStepsSbr;
  const FIXP_DBL *quantStepsOrig;
  const FIXP_DBL *nrgBorders;
  INT   numRegionsSbr;
  INT   numRegionsOrig;
  INT   numRegionsNrg;
  INVF_MODE regionSpace[5][5];
  INVF_MODE regionSpaceTransient[5][5];
  INT EnergyCompFactor[5];

}DETECTOR_PARAMETERS;

typedef struct
{
  FIXP_DBL  origQuotaMean[INVF_SMOOTHING_LENGTH+1];
  FIXP_DBL  sbrQuotaMean[INVF_SMOOTHING_LENGTH+1];
  FIXP_DBL  origQuotaMeanStrongest[INVF_SMOOTHING_LENGTH+1];
  FIXP_DBL  sbrQuotaMeanStrongest[INVF_SMOOTHING_LENGTH+1];

  FIXP_DBL origQuotaMeanFilt;
  FIXP_DBL sbrQuotaMeanFilt;
  FIXP_DBL origQuotaMeanStrongestFilt;
  FIXP_DBL sbrQuotaMeanStrongestFilt;

  FIXP_DBL origQuotaMax;
  FIXP_DBL sbrQuotaMax;

  FIXP_DBL avgNrg;
}DETECTOR_VALUES;



typedef struct
{
  INT numberOfStrongest;

  INT prevRegionSbr[MAX_NUM_NOISE_VALUES];
  INT prevRegionOrig[MAX_NUM_NOISE_VALUES];

  INT freqBandTableInvFilt[MAX_NUM_NOISE_VALUES];
  INT noDetectorBands;
  INT noDetectorBandsMax;

  const DETECTOR_PARAMETERS *detectorParams;

  INVF_MODE prevInvfMode[MAX_NUM_NOISE_VALUES];
  DETECTOR_VALUES detectorValues[MAX_NUM_NOISE_VALUES];

  FIXP_DBL nrgAvg;
  FIXP_DBL wmQmf[MAX_NUM_NOISE_VALUES];
}
SBR_INV_FILT_EST;

typedef SBR_INV_FILT_EST *HANDLE_SBR_INV_FILT_EST;

void
FDKsbrEnc_qmfInverseFilteringDetector(HANDLE_SBR_INV_FILT_EST hInvFilt,
                            FIXP_DBL ** quotaMatrix,
                            FIXP_DBL *nrgVector,
                            SCHAR    *indexVector,
                            INT startIndex,
                            INT stopIndex,
                            INT transientFlag,
                            INVF_MODE* infVec);

INT
FDKsbrEnc_initInvFiltDetector (HANDLE_SBR_INV_FILT_EST hInvFilt,
                       INT* freqBandTableDetector,
                       INT numDetectorBands,
                       UINT useSpeechConfig);

/* void deleteInvFiltDetector (HANDLE_SBR_INV_FILT_EST hInvFilt); */

INT
FDKsbrEnc_resetInvFiltDetector(HANDLE_SBR_INV_FILT_EST hInvFilt,
                     INT* freqBandTableDetector,
                     INT numDetectorBands);

#endif /* _QMF_INV_FILT_H */

