/*****************************  MPEG-4 AAC Decoder  **************************

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


   $Id$
   Author(s):   Christian Griebel
   Description: Error concealment structs and types

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef CONCEAL_TYPES_H
#define CONCEAL_TYPES_H



#include "machine_type.h"
#include "common_fix.h"

#include "rvlc_info.h"


#define CONCEAL_MAX_NUM_FADE_FACTORS      ( 16 )

 #define FIXP_CNCL        FIXP_DBL
 #define FL2FXCONST_CNCL  FL2FXCONST_DBL
 #define FX_DBL2FX_CNCL
 #define FX_CNCL2FX_DBL
 #define CNCL_FRACT_BITS  DFRACT_BITS

/* Warning: Do not ever change these values. */
typedef enum
{
  ConcealMethodNone  = -1,
  ConcealMethodMute  =  0,
  ConcealMethodNoise =  1,
  ConcealMethodInter =  2,
  ConcealMethodTonal =  3

} CConcealmentMethod;


typedef enum
{
  ConcealState_Ok,
  ConcealState_Single,
  ConcealState_FadeIn,
  ConcealState_Mute,
  ConcealState_FadeOut

} CConcealmentState;


typedef struct
{
  FIXP_SGL  fadeOutFactor[CONCEAL_MAX_NUM_FADE_FACTORS];
  FIXP_SGL  fadeInFactor [CONCEAL_MAX_NUM_FADE_FACTORS];

  CConcealmentMethod  method;

  int  numFadeOutFrames;
  int  numFadeInFrames;
  int  numMuteReleaseFrames;
  int  comfortNoiseLevel;

} CConcealParams;



typedef struct
{
  CConcealParams *pConcealParams;

  FIXP_CNCL spectralCoefficient[1024];
  SHORT     specScale[8];

  INT    iRandomPhase;
  INT    prevFrameOk[2];
  INT    cntFadeFrames;
  INT    cntValidFrames;

  SHORT aRvlcPreviousScaleFactor[RVLC_MAX_SFB];  /* needed once per channel */
  UCHAR aRvlcPreviousCodebook[RVLC_MAX_SFB];     /* needed once per channel */
  SCHAR rvlcPreviousScaleFactorOK;
  SCHAR rvlcPreviousBlockType;


  SCHAR  lastRenderMode;

  UCHAR  windowShape;
  UCHAR  windowSequence;
  UCHAR  lastWinGrpLen;

  CConcealmentState concealState;

} CConcealmentInfo;


#endif /* #ifndef CONCEAL_TYPES_H */
