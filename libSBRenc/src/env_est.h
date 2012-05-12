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
  \brief  Envelope estimation structs and prototypes $Revision: 37142 $
*/
#ifndef __ENV_EST_H
#define __ENV_EST_H

#include "sbr_def.h"
#include "sbr_encoder.h" /* SBR econfig structs */
#include "ps_main.h"
#include "bit_sbr.h"
#include "fram_gen.h"
#include "tran_det.h"
#include "code_env.h"
#include "ton_corr.h"

typedef struct
{
  FIXP_DBL  *rBuffer[QMF_MAX_TIME_SLOTS];
  FIXP_DBL  *iBuffer[QMF_MAX_TIME_SLOTS];

  FIXP_DBL  *p_YBuffer;

  FIXP_DBL  *YBuffer[QMF_MAX_TIME_SLOTS];
  int        YBufferScale[2];

  UCHAR envelopeCompensation[MAX_FREQ_COEFFS];
  UCHAR pre_transient_info[2];


  int YBufferWriteOffset;
  int YBufferSzShift;
  int rBufferReadOffset;

  int no_cols;
  int no_rows;
  int start_index;

  int time_slots;
  int time_step;
}
SBR_EXTRACT_ENVELOPE;
typedef SBR_EXTRACT_ENVELOPE *HANDLE_SBR_EXTRACT_ENVELOPE;

struct ENV_CHANNEL
{
  SBR_TRANSIENT_DETECTOR sbrTransientDetector;
  SBR_CODE_ENVELOPE sbrCodeEnvelope;
  SBR_CODE_ENVELOPE sbrCodeNoiseFloor;
  SBR_EXTRACT_ENVELOPE sbrExtractEnvelope;


  SBR_ENVELOPE_FRAME SbrEnvFrame;
  SBR_TON_CORR_EST   TonCorr;

  struct SBR_ENV_DATA encEnvData;

  int qmfScale;
  UCHAR fLevelProtect;
};
typedef struct ENV_CHANNEL *HANDLE_ENV_CHANNEL;

/************  Function Declarations ***************/

INT
FDKsbrEnc_CreateExtractSbrEnvelope (HANDLE_SBR_EXTRACT_ENVELOPE  hSbrCut,
                                    INT channel
                                   ,INT chInEl
                                   ,UCHAR* dynamic_RAM
                         );


INT
FDKsbrEnc_InitExtractSbrEnvelope (
                          HANDLE_SBR_EXTRACT_ENVELOPE hSbr,
                          int no_cols,
                          int no_rows,
                          int start_index,
                          int time_slots, int time_step, int tran_off,
                          ULONG statesInitFlag
                         ,int chInEl
                         ,UCHAR* dynamic_RAM
                         ,UINT sbrSyntaxFlags
                         );

void FDKsbrEnc_deleteExtractSbrEnvelope (HANDLE_SBR_EXTRACT_ENVELOPE hSbrCut);

typedef struct {
    FREQ_RES res[MAX_NUM_NOISE_VALUES];
    int maxQuantError;

} SBR_FRAME_TEMP_DATA;

typedef struct {
    const SBR_FRAME_INFO *frame_info;
    FIXP_DBL noiseFloor[MAX_NUM_NOISE_VALUES];
    SCHAR sfb_nrg_coupling[MAX_NUM_ENVELOPE_VALUES]; /* only used if stereomode = SWITCH_L_R_C */
    SCHAR sfb_nrg[MAX_NUM_ENVELOPE_VALUES];
    SCHAR noise_level_coupling[MAX_NUM_NOISE_VALUES]; /* only used if stereomode = SWITCH_L_R_C */
    SCHAR noise_level[MAX_NUM_NOISE_VALUES];
    UCHAR transient_info[3];
    UCHAR nEnvelopes;
} SBR_ENV_TEMP_DATA;

/*
 * Extract features from QMF data. Afterwards, the QMF data is not required anymore.
 */
void
FDKsbrEnc_extractSbrEnvelope1(
                   HANDLE_SBR_CONFIG_DATA    h_con,
                   HANDLE_SBR_HEADER_DATA    sbrHeaderData,
                   HANDLE_SBR_BITSTREAM_DATA sbrBitstreamData,
                   HANDLE_ENV_CHANNEL        h_envChan,
                   HANDLE_COMMON_DATA        cmonData,
                   SBR_ENV_TEMP_DATA   *eData,
                   SBR_FRAME_TEMP_DATA *fData
                    );


/*
 * Process the previously features extracted by FDKsbrEnc_extractSbrEnvelope1
 * and create/encode SBR envelopes.
 */
void
FDKsbrEnc_extractSbrEnvelope2(
                   HANDLE_SBR_CONFIG_DATA     h_con,
                   HANDLE_SBR_HEADER_DATA     sbrHeaderData,
                   HANDLE_PARAMETRIC_STEREO   hParametricStereo,
                   HANDLE_SBR_BITSTREAM_DATA  sbrBitstreamData,
                   HANDLE_ENV_CHANNEL         sbrEnvChannel0,
                   HANDLE_ENV_CHANNEL         sbrEnvChannel1,
                   HANDLE_COMMON_DATA         cmonData,
                   SBR_ENV_TEMP_DATA         *eData,
                   SBR_FRAME_TEMP_DATA       *fData,
                   int                        clearOutput
                   );

INT
FDKsbrEnc_GetEnvEstDelay(HANDLE_SBR_EXTRACT_ENVELOPE hSbr);

#endif
