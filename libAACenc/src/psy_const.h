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
   Initial author:       M.Werner
   contents/description: Global psychoaccoustic constants

******************************************************************************/
#ifndef _PSYCONST_H
#define _PSYCONST_H


#define TRUE  1
#define FALSE 0

  #define TRANS_FAC         8  /* encoder short long ratio */

#define FRAME_MAXLEN_SHORT    ((1024)/TRANS_FAC)
#define FRAME_LEN_SHORT_128   ((1024)/TRANS_FAC)
#define FRAME_LEN_SHORT_120   (FRAME_LEN_LONG_960/TRANS_FAC)

/* Filterbank type*/
enum FB_TYPE {
  FB_LC  = 0,
  FB_LD  = 1,
  FB_ELD = 2
};

/* Block types */
#define N_BLOCKTYPES 6
enum
{
  LONG_WINDOW = 0,
  START_WINDOW,
  SHORT_WINDOW,
  STOP_WINDOW,
  _LOWOV_WINDOW, /* Do not use this block type out side of block_switch.cpp */
  WRONG_WINDOW
};

/* Window shapes */
enum
{
  SINE_WINDOW = 0,
  KBD_WINDOW  = 1,
  LOL_WINDOW = 2 /* Low OverLap window shape for AAC-LD */
};

/*
  MS stuff
*/
enum
{
  SI_MS_MASK_NONE = 0,
  SI_MS_MASK_SOME = 1,
  SI_MS_MASK_ALL  = 2
};


  #define MAX_NO_OF_GROUPS   4
  #define MAX_SFB_LONG       51  /* 51 for a memory optimized implementation, maybe 64 for convenient debugging */
  #define MAX_SFB_SHORT      15  /* 15 for a memory optimized implementation, maybe 16 for convenient debugging */

#define MAX_SFB         (MAX_SFB_SHORT > MAX_SFB_LONG ? MAX_SFB_SHORT : MAX_SFB_LONG)   /* = 51 */
#define MAX_GROUPED_SFB (MAX_NO_OF_GROUPS*MAX_SFB_SHORT > MAX_SFB_LONG ? \
                         MAX_NO_OF_GROUPS*MAX_SFB_SHORT : MAX_SFB_LONG) /* = 60 */

#define MAX_INPUT_BUFFER_SIZE  (2*(1024))   /* 2048 */


#define PCM_LEVEL             1.0f
#define NORM_PCM              (PCM_LEVEL/32768.0f)
#define NORM_PCM_ENERGY       (NORM_PCM*NORM_PCM)
#define LOG_NORM_PCM          -15

#define TNS_PREDGAIN_SCALE    (1000)

#define LFE_LOWPASS_LINE      12

#endif /* _PSYCONST_H */
