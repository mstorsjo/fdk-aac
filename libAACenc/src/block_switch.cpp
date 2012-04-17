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

/****************** Includes *****************************/

#include "block_switch.h"
#include "genericStds.h"


#define LOWOV_WINDOW _LOWOV_WINDOW

/**************** internal function prototypes ***********/

static FIXP_DBL FDKaacEnc_GetWindowEnergy(const FIXP_DBL in[], const INT blSwWndIdx);

static void FDKaacEnc_CalcWindowEnergy( BLOCK_SWITCHING_CONTROL *RESTRICT blockSwitchingControl,
                              INT                      windowLen);


/****************** Constants *****************************/
/*                                                LONG         START        SHORT         STOP         LOWOV                  */
static const INT blockType2windowShape[2][5] = { {SINE_WINDOW, KBD_WINDOW,  WRONG_WINDOW, SINE_WINDOW, KBD_WINDOW},     /* LD */
                                                 {KBD_WINDOW,  SINE_WINDOW, SINE_WINDOW,  KBD_WINDOW,  WRONG_WINDOW} }; /* LC */

/* IIR high pass coeffs */

#ifndef SINETABLE_16BIT

static const FIXP_DBL hiPassCoeff[BLOCK_SWITCHING_IIR_LEN]=
{
  FL2FXCONST_DBL(-0.5095),FL2FXCONST_DBL(0.7548)
};

static const FIXP_DBL accWindowNrgFac = FL2FXCONST_DBL(0.3f);                   /* factor for accumulating filtered window energies */
static const FIXP_DBL oneMinusAccWindowNrgFac = FL2FXCONST_DBL(0.7f);
/* static const float attackRatio = 10.0; */                                    /* lower ratio limit for attacks */
static const FIXP_DBL invAttackRatio = FL2FXCONST_DBL(0.1f);                    /* inverted lower ratio limit for attacks */

/* The next constants are scaled, because they are used for comparison with scaled values*/
/* minimum energy for attacks */
static const FIXP_DBL minAttackNrg = (FL2FXCONST_DBL(1e+6f*NORM_PCM_ENERGY)>>BLOCK_SWITCH_ENERGY_SHIFT); /* minimum energy for attacks */

#else

static const FIXP_SGL hiPassCoeff[BLOCK_SWITCHING_IIR_LEN]=
{
  FL2FXCONST_SGL(-0.5095),FL2FXCONST_SGL(0.7548)
};

static const FIXP_DBL accWindowNrgFac = FL2FXCONST_DBL(0.3f);                   /* factor for accumulating filtered window energies */
static const FIXP_SGL oneMinusAccWindowNrgFac = FL2FXCONST_SGL(0.7f);
/* static const float attackRatio = 10.0; */                                    /* lower ratio limit for attacks */
static const FIXP_SGL invAttackRatio = FL2FXCONST_SGL(0.1f);                    /* inverted lower ratio limit for attacks */
/* minimum energy for attacks */
static const FIXP_DBL minAttackNrg = (FL2FXCONST_DBL(1e+6f*NORM_PCM_ENERGY)>>BLOCK_SWITCH_ENERGY_SHIFT); /* minimum energy for attacks */

#endif

/**************** internal function prototypes ***********/

static INT FDKaacEnc_GetWindowIndex(INT blockSwWindowIndex);

static FIXP_DBL FDKaacEnc_GetWindowEnergy(const FIXP_DBL in[], const INT shortWndIdx);

static void FDKaacEnc_CalcWindowEnergy( BLOCK_SWITCHING_CONTROL *RESTRICT blockSwitchingControl,
                                        INT windowLen);



/****************** Routines ****************************/
void FDKaacEnc_InitBlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControl, INT isLowDelay)
{
  /* note: the pointer to timeSignal can be zeroed here, because it is initialized for every call
           to FDKaacEnc_BlockSwitching anew */
  FDKmemclear (blockSwitchingControl, sizeof(BLOCK_SWITCHING_CONTROL));

  if (isLowDelay)
  {
    blockSwitchingControl->nBlockSwitchWindows = 4;
    blockSwitchingControl->allowShortFrames    = 0;
    blockSwitchingControl->allowLookAhead      = 0;
  }
  else
  {
    blockSwitchingControl->nBlockSwitchWindows = 8;
    blockSwitchingControl->allowShortFrames    = 1;
    blockSwitchingControl->allowLookAhead      = 1;
  }

  blockSwitchingControl->noOfGroups            = MAX_NO_OF_GROUPS;

  /* Initialize startvalue for blocktype */
  blockSwitchingControl->lastWindowSequence    = LONG_WINDOW;
  blockSwitchingControl->windowShape           = blockType2windowShape[blockSwitchingControl->allowShortFrames][blockSwitchingControl->lastWindowSequence];

}

static const INT suggestedGroupingTable[TRANS_FAC][MAX_NO_OF_GROUPS] =
{
    /* Attack in Window 0 */ {1,  3,  3,  1},
    /* Attack in Window 1 */ {1,  1,  3,  3},
    /* Attack in Window 2 */ {2,  1,  3,  2},
    /* Attack in Window 3 */ {3,  1,  3,  1},
    /* Attack in Window 4 */ {3,  1,  1,  3},
    /* Attack in Window 5 */ {3,  2,  1,  2},
    /* Attack in Window 6 */ {3,  3,  1,  1},
    /* Attack in Window 7 */ {3,  3,  1,  1}
};

/* change block type depending on current blocktype and whether there's an attack */
/* assume no look-ahead */
static const INT chgWndSq[2][N_BLOCKTYPES] =
{
  /*             LONG WINDOW   START_WINDOW  SHORT_WINDOW  STOP_WINDOW,  LOWOV_WINDOW, WRONG_WINDOW */
  /*no attack*/ {LONG_WINDOW,  STOP_WINDOW,  WRONG_WINDOW, LONG_WINDOW,  STOP_WINDOW , WRONG_WINDOW },
  /*attack   */ {START_WINDOW, LOWOV_WINDOW, WRONG_WINDOW, START_WINDOW, LOWOV_WINDOW, WRONG_WINDOW }
};

/* change block type depending on current blocktype and whether there's an attack */
/* assume look-ahead */
static const INT chgWndSqLkAhd[2][2][N_BLOCKTYPES] =
{
  /*attack         LONG WINDOW    START_WINDOW   SHORT_WINDOW   STOP_WINDOW   LOWOV_WINDOW, WRONG_WINDOW */  /* last attack */
  /*no attack*/ { {LONG_WINDOW,   SHORT_WINDOW,  STOP_WINDOW,   LONG_WINDOW,  WRONG_WINDOW, WRONG_WINDOW},   /* no attack   */
  /*attack   */   {START_WINDOW,  SHORT_WINDOW,  SHORT_WINDOW,  START_WINDOW, WRONG_WINDOW, WRONG_WINDOW} }, /* no attack   */
  /*no attack*/ { {LONG_WINDOW,   SHORT_WINDOW,  SHORT_WINDOW,  LONG_WINDOW,  WRONG_WINDOW, WRONG_WINDOW},   /* attack      */
  /*attack   */   {START_WINDOW,  SHORT_WINDOW,  SHORT_WINDOW,  START_WINDOW, WRONG_WINDOW, WRONG_WINDOW} }  /* attack      */
};

int FDKaacEnc_BlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControl, const INT granuleLength, const int isLFE)
{
    UINT i;
    FIXP_DBL enM1, enMax;

    UINT nBlockSwitchWindows = blockSwitchingControl->nBlockSwitchWindows;

    /* for LFE : only LONG window allowed */
    if (isLFE) {

      /* case LFE: */
      /* only long blocks, always use sine windows (MPEG2 AAC, MPEG4 AAC) */
      blockSwitchingControl->lastWindowSequence = LONG_WINDOW;
      blockSwitchingControl->windowShape    = SINE_WINDOW;
      blockSwitchingControl->noOfGroups     = 1;
      blockSwitchingControl->groupLen[0]    = 1;

      return(0);
    };

    /* Save current attack index as last attack index */
    blockSwitchingControl->lastattack = blockSwitchingControl->attack;
    blockSwitchingControl->lastAttackIndex = blockSwitchingControl->attackIndex;

    /* Save current window energy as last window energy */
    FDKmemcpy(blockSwitchingControl->windowNrg[0], blockSwitchingControl->windowNrg[1], sizeof(blockSwitchingControl->windowNrg[0]));
    FDKmemcpy(blockSwitchingControl->windowNrgF[0], blockSwitchingControl->windowNrgF[1], sizeof(blockSwitchingControl->windowNrgF[0]));

    if (blockSwitchingControl->allowShortFrames)
    {
      /* Calculate suggested grouping info for the last frame */

      /* Reset grouping info */
      FDKmemclear (blockSwitchingControl->groupLen, sizeof(blockSwitchingControl->groupLen));

      /* Set grouping info */
      blockSwitchingControl->noOfGroups = MAX_NO_OF_GROUPS;

      FDKmemcpy(blockSwitchingControl->groupLen, suggestedGroupingTable[blockSwitchingControl->lastAttackIndex], sizeof(blockSwitchingControl->groupLen));

      if (blockSwitchingControl->attack == TRUE)
          blockSwitchingControl->maxWindowNrg = FDKaacEnc_GetWindowEnergy(blockSwitchingControl->windowNrg[0], blockSwitchingControl->lastAttackIndex);
      else
          blockSwitchingControl->maxWindowNrg = FL2FXCONST_DBL(0.0);

    }


    /* Calculate unfiltered and filtered energies in subwindows and combine to segments */
    FDKaacEnc_CalcWindowEnergy(blockSwitchingControl, granuleLength>>(nBlockSwitchWindows==4? 2:3 ));

    /* now calculate if there is an attack */

    /* reset attack */
    blockSwitchingControl->attack = FALSE;

    /* look for attack */
    enMax = FL2FXCONST_DBL(0.0f);
    enM1 = blockSwitchingControl->windowNrgF[0][nBlockSwitchWindows-1];

    for (i=0; i<nBlockSwitchWindows; i++) {
        FIXP_DBL tmp = fMultDiv2(oneMinusAccWindowNrgFac, blockSwitchingControl->accWindowNrg);
        blockSwitchingControl->accWindowNrg = fMultAdd(tmp, accWindowNrgFac, enM1) ;

        if (fMult(blockSwitchingControl->windowNrgF[1][i],invAttackRatio) > blockSwitchingControl->accWindowNrg ) {
            blockSwitchingControl->attack = TRUE;
            blockSwitchingControl->attackIndex = i;
        }
        enM1 = blockSwitchingControl->windowNrgF[1][i];
        enMax = fixMax(enMax, enM1);
    }


    if (enMax < minAttackNrg) blockSwitchingControl->attack = FALSE;

    /* Check if attack spreads over frame border */
    if((blockSwitchingControl->attack == FALSE) && (blockSwitchingControl->lastattack == TRUE)) {
        /* if attack is in last window repeat SHORT_WINDOW */
        if ( ((blockSwitchingControl->windowNrgF[0][nBlockSwitchWindows-1]>>4) > fMult((FIXP_DBL)(10<<(DFRACT_BITS-1-4)), blockSwitchingControl->windowNrgF[1][1]))
           && (blockSwitchingControl->lastAttackIndex == (INT)nBlockSwitchWindows-1)
        )
        {
            blockSwitchingControl->attack = TRUE;
            blockSwitchingControl->attackIndex = 0;
        }
    }


    if(blockSwitchingControl->allowLookAhead)
    {


      blockSwitchingControl->lastWindowSequence =
        chgWndSqLkAhd[blockSwitchingControl->lastattack][blockSwitchingControl->attack][blockSwitchingControl->lastWindowSequence];
    }
    else
    {
      /* Low Delay */
      blockSwitchingControl->lastWindowSequence =
        chgWndSq[blockSwitchingControl->attack][blockSwitchingControl->lastWindowSequence];
    }


    /* update window shape */
    blockSwitchingControl->windowShape = blockType2windowShape[blockSwitchingControl->allowShortFrames][blockSwitchingControl->lastWindowSequence];

    return(0);
}



static FIXP_DBL FDKaacEnc_GetWindowEnergy(const FIXP_DBL in[], const INT blSwWndIdx)
{
/* For coherency, change FDKaacEnc_GetWindowEnergy() to calcluate the energy for a block switching analysis windows,
   not for a short block. The same is done FDKaacEnc_CalcWindowEnergy(). The result of FDKaacEnc_GetWindowEnergy()
   is used for a comparision of the max energy of left/right channel. */

  return in[blSwWndIdx];

}


static void FDKaacEnc_CalcWindowEnergy(BLOCK_SWITCHING_CONTROL *RESTRICT blockSwitchingControl, INT windowLen)
{
    INT  i;
    UINT w;

    FIXP_SGL hiPassCoeff0 = hiPassCoeff[0];
    FIXP_SGL hiPassCoeff1 = hiPassCoeff[1];

    INT_PCM *timeSignal = blockSwitchingControl->timeSignal;

    /* sum up scalarproduct of timesignal as windowed Energies */
    for (w=0; w < blockSwitchingControl->nBlockSwitchWindows; w++) {

        FIXP_DBL temp_windowNrg  = FL2FXCONST_DBL(0.0f);
        FIXP_DBL temp_windowNrgF = FL2FXCONST_DBL(0.0f);
        FIXP_DBL temp_iirState0  = blockSwitchingControl->iirStates[0];
        FIXP_DBL temp_iirState1  = blockSwitchingControl->iirStates[1];

        /* windowNrg = sum(timesample^2) */
        for(i=0;i<windowLen;i++)
        {

            FIXP_DBL tempUnfiltered, tempFiltred, t1, t2;
            /* tempUnfiltered is scaled with 1 to prevent overflows during calculation of tempFiltred */
#if SAMPLE_BITS == DFRACT_BITS
            tempUnfiltered = (FIXP_DBL) *timeSignal++ >> 1;
#else
            tempUnfiltered = (FIXP_DBL) *timeSignal++ << (DFRACT_BITS-SAMPLE_BITS-1);
#endif
            t1 = fMultDiv2(hiPassCoeff1, tempUnfiltered-temp_iirState0);
            t2 = fMultDiv2(hiPassCoeff0, temp_iirState1);
            tempFiltred = (t1 - t2) << 1;

            temp_iirState0 = tempUnfiltered;
            temp_iirState1 = tempFiltred;

            /* subtract 2 from overallscaling (BLOCK_SWITCH_ENERGY_SHIFT)
             * because tempUnfiltered was already scaled with 1 (is 2 after squaring)
             * subtract 1 from overallscaling (BLOCK_SWITCH_ENERGY_SHIFT)
             * because of fMultDiv2 is doing a scaling by one */
            temp_windowNrg += fPow2Div2(tempUnfiltered) >> (BLOCK_SWITCH_ENERGY_SHIFT - 1 - 2);
            temp_windowNrgF += fPow2Div2(tempFiltred) >> (BLOCK_SWITCH_ENERGY_SHIFT - 1 - 2);
        }
        blockSwitchingControl->windowNrg[1][w]  = temp_windowNrg;
        blockSwitchingControl->windowNrgF[1][w] = temp_windowNrgF;
        blockSwitchingControl->iirStates[0]     = temp_iirState0;
        blockSwitchingControl->iirStates[1]     = temp_iirState1;
    }
}


static const UCHAR synchronizedBlockTypeTable[5][5] =
{
  /*                  LONG_WINDOW   START_WINDOW  SHORT_WINDOW  STOP_WINDOW   LOWOV_WINDOW*/
  /* LONG_WINDOW  */ {LONG_WINDOW,  START_WINDOW, SHORT_WINDOW, STOP_WINDOW,  LOWOV_WINDOW},
  /* START_WINDOW */ {START_WINDOW, START_WINDOW, SHORT_WINDOW, SHORT_WINDOW, LOWOV_WINDOW},
  /* SHORT_WINDOW */ {SHORT_WINDOW, SHORT_WINDOW, SHORT_WINDOW, SHORT_WINDOW, WRONG_WINDOW},
  /* STOP_WINDOW  */ {STOP_WINDOW,  SHORT_WINDOW, SHORT_WINDOW, STOP_WINDOW,  LOWOV_WINDOW},
  /* LOWOV_WINDOW */ {LOWOV_WINDOW, LOWOV_WINDOW, WRONG_WINDOW, LOWOV_WINDOW, LOWOV_WINDOW},
};

int FDKaacEnc_SyncBlockSwitching (
      BLOCK_SWITCHING_CONTROL *blockSwitchingControlLeft,
      BLOCK_SWITCHING_CONTROL *blockSwitchingControlRight,
      const INT nChannels,
      const INT commonWindow )
{
  UCHAR patchType = LONG_WINDOW;

  if( nChannels == 2 && commonWindow == TRUE)
  {
    /* could be better with a channel loop (need a handle to psy_data) */
    /* get suggested Block Types and synchronize */
    patchType = synchronizedBlockTypeTable[patchType][blockSwitchingControlLeft->lastWindowSequence];
    patchType = synchronizedBlockTypeTable[patchType][blockSwitchingControlRight->lastWindowSequence];

    /* sanity check (no change from low overlap window to short winow and vice versa) */
    if (patchType == WRONG_WINDOW)
      return -1; /* mixed up AAC-LC and AAC-LD */

    /* Set synchronized Blocktype */
    blockSwitchingControlLeft->lastWindowSequence  = patchType;
    blockSwitchingControlRight->lastWindowSequence = patchType;

    /* update window shape */
    blockSwitchingControlLeft->windowShape  = blockType2windowShape[blockSwitchingControlLeft->allowShortFrames][blockSwitchingControlLeft->lastWindowSequence];
    blockSwitchingControlRight->windowShape = blockType2windowShape[blockSwitchingControlLeft->allowShortFrames][blockSwitchingControlRight->lastWindowSequence];
  }

  if (blockSwitchingControlLeft->allowShortFrames)
  {
    int i;

    if( nChannels == 2 )
    {
      if (commonWindow == TRUE)
      {
        /* Synchronize grouping info */
        int windowSequenceLeftOld  = blockSwitchingControlLeft->lastWindowSequence;
        int windowSequenceRightOld = blockSwitchingControlRight->lastWindowSequence;

        /* Long Blocks */
        if(patchType != SHORT_WINDOW) {
          /* Set grouping info */
          blockSwitchingControlLeft->noOfGroups   = 1;
          blockSwitchingControlRight->noOfGroups  = 1;
          blockSwitchingControlLeft->groupLen[0]  = 1;
          blockSwitchingControlRight->groupLen[0] = 1;

          for (i = 1; i < MAX_NO_OF_GROUPS; i++)
          {
            blockSwitchingControlLeft->groupLen[i]  = 0;
            blockSwitchingControlRight->groupLen[i] = 0;
          }
        }

        /* Short Blocks */
        else {
          /* in case all two channels were detected as short-blocks before syncing, use the grouping of channel with higher maxWindowNrg */
          if( (windowSequenceLeftOld  == SHORT_WINDOW) &&
	            (windowSequenceRightOld == SHORT_WINDOW) )
          {
            if(blockSwitchingControlLeft->maxWindowNrg > blockSwitchingControlRight->maxWindowNrg) {
	            /* Left Channel wins */
	            blockSwitchingControlRight->noOfGroups = blockSwitchingControlLeft->noOfGroups;
	            for (i = 0; i < MAX_NO_OF_GROUPS; i++){
	              blockSwitchingControlRight->groupLen[i] = blockSwitchingControlLeft->groupLen[i];
	            }
            }
            else {
	            /* Right Channel wins */
	            blockSwitchingControlLeft->noOfGroups = blockSwitchingControlRight->noOfGroups;
	            for (i = 0; i < MAX_NO_OF_GROUPS; i++){
	              blockSwitchingControlLeft->groupLen[i] = blockSwitchingControlRight->groupLen[i];
	            }
            }
          }
          else if ( (windowSequenceLeftOld  == SHORT_WINDOW) &&
                    (windowSequenceRightOld != SHORT_WINDOW) )
          {
            /* else use grouping of short-block channel */
            blockSwitchingControlRight->noOfGroups = blockSwitchingControlLeft->noOfGroups;
            for (i = 0; i < MAX_NO_OF_GROUPS; i++){
              blockSwitchingControlRight->groupLen[i] = blockSwitchingControlLeft->groupLen[i];
            }
          }
          else if ( (windowSequenceRightOld == SHORT_WINDOW) &&
		                (windowSequenceLeftOld  != SHORT_WINDOW) )
          {
            blockSwitchingControlLeft->noOfGroups = blockSwitchingControlRight->noOfGroups;
            for (i = 0; i < MAX_NO_OF_GROUPS; i++){
              blockSwitchingControlLeft->groupLen[i] = blockSwitchingControlRight->groupLen[i];
            }
          } else {
            /* syncing a start and stop window ... */
            /* use a standard grouping,
             best grouping still to be determined! rtb 2006 06 07 */
            blockSwitchingControlLeft->noOfGroups  = blockSwitchingControlRight->noOfGroups  = 2;
            blockSwitchingControlLeft->groupLen[0] = blockSwitchingControlRight->groupLen[0] = 4;
            blockSwitchingControlLeft->groupLen[1] = blockSwitchingControlRight->groupLen[1] = 4;
          }
        } /* Short Blocks */
      }
      else {
        /* stereo, no common window */
        if (blockSwitchingControlLeft->lastWindowSequence!=SHORT_WINDOW){
          blockSwitchingControlLeft->noOfGroups  = 1;
          blockSwitchingControlLeft->groupLen[0] = 1;
          for (i = 1; i < MAX_NO_OF_GROUPS; i++)
          {
            blockSwitchingControlLeft->groupLen[i] = 0;
          }
        }
        if (blockSwitchingControlRight->lastWindowSequence!=SHORT_WINDOW){
          blockSwitchingControlRight->noOfGroups  = 1;
          blockSwitchingControlRight->groupLen[0] = 1;
          for (i = 1; i < MAX_NO_OF_GROUPS; i++)
          {
            blockSwitchingControlRight->groupLen[i] = 0;
          }
        }
      } /* common window */
    } else {
      /* Mono */
      if (blockSwitchingControlLeft->lastWindowSequence!=SHORT_WINDOW){
        blockSwitchingControlLeft->noOfGroups  = 1;
        blockSwitchingControlLeft->groupLen[0] = 1;

        for (i = 1; i < MAX_NO_OF_GROUPS; i++)
        {
          blockSwitchingControlLeft->groupLen[i] = 0;
        }
      }
    }
  } /* allowShortFrames */

#ifdef  BLOCK_SWITCH_DEBUG_INFO
  FDKprintf("l:%d\tr:%d\n", blockSwitchingControlLeft->lastWindowSequence, blockSwitchingControlRight->lastWindowSequence);
#endif

  /* Translate LOWOV_WINDOW block type to a meaningful window shape. */
  if ( ! blockSwitchingControlLeft->allowShortFrames ) {
    if ( blockSwitchingControlLeft->lastWindowSequence != LONG_WINDOW
      && blockSwitchingControlLeft->lastWindowSequence != STOP_WINDOW )
    {
      blockSwitchingControlLeft->lastWindowSequence = LONG_WINDOW;
      blockSwitchingControlLeft->windowShape = LOL_WINDOW;
    }
  }
  if (nChannels == 2) {
    if ( ! blockSwitchingControlRight->allowShortFrames ) {
      if ( blockSwitchingControlRight->lastWindowSequence != LONG_WINDOW
        && blockSwitchingControlRight->lastWindowSequence != STOP_WINDOW )
      {
        blockSwitchingControlRight->lastWindowSequence = LONG_WINDOW;
        blockSwitchingControlRight->windowShape = LOL_WINDOW;
      }
    }
  }

  return 0;
}


