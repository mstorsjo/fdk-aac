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
\brief Declaration of constant tables
$Revision: 37577 $
*/
#ifndef __SBR_ROM_H
#define __SBR_ROM_H

#include "sbr_def.h"
#include "sbr_encoder.h"

#include "ps_main.h"

/*
  huffman tables
*/
extern const INT           v_Huff_envelopeLevelC10T[121];
extern const UCHAR         v_Huff_envelopeLevelL10T[121];
extern const INT           v_Huff_envelopeLevelC10F[121];
extern const UCHAR         v_Huff_envelopeLevelL10F[121];
extern const INT           bookSbrEnvBalanceC10T[49];
extern const UCHAR         bookSbrEnvBalanceL10T[49];
extern const INT           bookSbrEnvBalanceC10F[49];
extern const UCHAR         bookSbrEnvBalanceL10F[49];
extern const INT           v_Huff_envelopeLevelC11T[63];
extern const UCHAR         v_Huff_envelopeLevelL11T[63];
extern const INT           v_Huff_envelopeLevelC11F[63];
extern const UCHAR         v_Huff_envelopeLevelL11F[63];
extern const INT           bookSbrEnvBalanceC11T[25];
extern const UCHAR         bookSbrEnvBalanceL11T[25];
extern const INT           bookSbrEnvBalanceC11F[25];
extern const UCHAR         bookSbrEnvBalanceL11F[25];
extern const INT           v_Huff_NoiseLevelC11T[63];
extern const UCHAR         v_Huff_NoiseLevelL11T[63];
extern const INT           bookSbrNoiseBalanceC11T[25];
extern const UCHAR         bookSbrNoiseBalanceL11T[25];

#define SBRENC_AACLC_TUNING_SIZE   124
#define SBRENC_AACELD_TUNING_SIZE   35
#define SBRENC_AACELD2_TUNING_SIZE  31

#define SBRENC_TUNING_SIZE (SBRENC_AACLC_TUNING_SIZE + SBRENC_AACELD_TUNING_SIZE)

extern const sbrTuningTable_t sbrTuningTable[SBRENC_TUNING_SIZE];

extern const psTuningTable_t psTuningTable[4];


#endif
