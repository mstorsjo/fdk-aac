/*****************************  MPEG-4 AAC Encoder  **************************

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


   $Id$
   Author(s):   M. Werner
   Description: Block switching

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef _BLOCK_SWITCH_H
#define _BLOCK_SWITCH_H

#include "common_fix.h"

#include "psy_const.h"

/****************** Defines ******************************/
  #define BLOCK_SWITCH_WINDOWS    8                   /* number of windows for energy calculation */

#define BLOCK_SWITCHING_IIR_LEN    2                  /* Length of HighPass-IIR-Filter for Attack-Detection */
#define BLOCK_SWITCH_ENERGY_SHIFT  7                  /* should be logDualis(BLOCK_SWITCH_WINDOW_LEN) to avoid overflow in windowNrgs. */

#define LAST_WINDOW 0
#define THIS_WINDOW 1


/****************** Structures ***************************/
typedef struct{
  INT_PCM *timeSignal;
  INT   lastWindowSequence;
  INT   windowShape;
  INT   lastWindowShape;
  UINT  nBlockSwitchWindows;                       /* number of windows for energy calculation */
  INT   attack;
  INT   lastattack;
  INT   attackIndex;
  INT   lastAttackIndex;
  INT   allowShortFrames;                          /* for Low Delay, don't allow short frames */
  INT   allowLookAhead;                            /* for Low Delay, don't do look-ahead */
  INT   noOfGroups;
  INT   groupLen[MAX_NO_OF_GROUPS];
  FIXP_DBL maxWindowNrg;                           /* max energy in subwindows */

  FIXP_DBL windowNrg[2][BLOCK_SWITCH_WINDOWS];     /* time signal energy in Subwindows (last and current) */
  FIXP_DBL windowNrgF[2][BLOCK_SWITCH_WINDOWS];    /* filtered time signal energy in segments (last and current) */
  FIXP_DBL accWindowNrg;                           /* recursively accumulated windowNrgF */

  FIXP_DBL iirStates[BLOCK_SWITCHING_IIR_LEN];     /* filter delay-line */

} BLOCK_SWITCHING_CONTROL;





void FDKaacEnc_InitBlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControl, INT isLowDelay);

int FDKaacEnc_BlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControl, const INT granuleLength, const int isLFE);

int FDKaacEnc_SyncBlockSwitching(
      BLOCK_SWITCHING_CONTROL *blockSwitchingControlLeft,
      BLOCK_SWITCHING_CONTROL *blockSwitchingControlRight,
      const INT noOfChannels,
      const INT commonWindow);

#endif  /* #ifndef _BLOCK_SWITCH_H */
