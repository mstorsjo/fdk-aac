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
   Description: Dynamic range control (DRC) decoder tool for SBR

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "sbrdec_drc.h"


/* DRC - Offset table for QMF interpolation. */
static const int offsetTab[2][16] =
{
  { 0, 4, 8, 12, 16, 20, 24, 28, 0, 0, 0, 0, 0, 0, 0, 0 },  /* 1024 framing */
  { 0, 4, 8, 12, 16, 19, 22, 26, 0, 0, 0, 0, 0, 0, 0, 0 }   /*  960 framing */
};

/*!
  \brief Initialize DRC QMF factors

  \hDrcData Handle to DRC channel data.

  \return none
*/
void sbrDecoder_drcInitChannel (
    HANDLE_SBR_DRC_CHANNEL  hDrcData )
{
  int band;

  if (hDrcData == NULL) {
    return;
  }

  for (band = 0; band < (64); band++) {
    hDrcData->prevFact_mag[band] = (FIXP_DBL)MAXVAL_DBL /*FL2FXCONST_DBL(1.0f)*/;
  }

  for (band = 0; band < SBRDEC_MAX_DRC_BANDS; band++) {
    hDrcData->currFact_mag[band] = (FIXP_DBL)MAXVAL_DBL /*FL2FXCONST_DBL(1.0f)*/;
    hDrcData->nextFact_mag[band] = (FIXP_DBL)MAXVAL_DBL /*FL2FXCONST_DBL(1.0f)*/;
  }

  hDrcData->prevFact_exp = 0;
  hDrcData->currFact_exp = 0;
  hDrcData->nextFact_exp = 0;

  hDrcData->numBandsCurr = 0;
  hDrcData->numBandsNext = 0;

  hDrcData->winSequenceCurr = 0;
  hDrcData->winSequenceNext = 0;

  hDrcData->drcInterpolationSchemeCurr = 0;
  hDrcData->drcInterpolationSchemeNext = 0;

  hDrcData->enable = 0;
}


/*!
  \brief Swap DRC QMF scaling factors after they have been applied.

  \hDrcData Handle to DRC channel data.

  \return none
*/
void sbrDecoder_drcUpdateChannel (
    HANDLE_SBR_DRC_CHANNEL  hDrcData )
{
  if (hDrcData == NULL) {
    return;
  }
  if (hDrcData->enable != 1) {
    return;
  }

  /* swap previous data */
  FDKmemcpy( hDrcData->currFact_mag,
             hDrcData->nextFact_mag,
             SBRDEC_MAX_DRC_BANDS * sizeof(FIXP_DBL) );

  hDrcData->currFact_exp = hDrcData->nextFact_exp;

  hDrcData->numBandsCurr = hDrcData->numBandsNext;

  FDKmemcpy( hDrcData->bandTopCurr,
             hDrcData->bandTopNext,
             SBRDEC_MAX_DRC_BANDS * sizeof(USHORT) );

  hDrcData->drcInterpolationSchemeCurr = hDrcData->drcInterpolationSchemeNext;

  hDrcData->winSequenceCurr = hDrcData->winSequenceNext;
}


/*!
  \brief Apply DRC factors slot based.

  \hDrcData Handle to DRC channel data.
  \qmfRealSlot Pointer to real valued QMF data of one time slot.
  \qmfImagSlot Pointer to the imaginary QMF data of one time slot.
  \col Number of the time slot.
  \numQmfSubSamples Total number of time slots for one frame.
  \scaleFactor Pointer to the out scale factor of the time slot.

  \return None.
*/
void sbrDecoder_drcApplySlot (
    HANDLE_SBR_DRC_CHANNEL  hDrcData,
    FIXP_DBL   *qmfRealSlot,
    FIXP_DBL   *qmfImagSlot,
    int  col,
    int  numQmfSubSamples,
    int  maxShift
  )
{
  const int *offset;

  int band, bottomMdct, topMdct, bin, useLP;
  int indx = numQmfSubSamples - (numQmfSubSamples >> 1) - 10;   /* l_border */
  int frameLenFlag = (numQmfSubSamples == 30) ? 1 : 0;

  const FIXP_DBL *fact_mag = NULL;
  INT fact_exp = 0;
  UINT numBands = 0;
  USHORT *bandTop = NULL;
  int shortDrc = 0;

  FIXP_DBL alphaValue = FL2FXCONST_DBL(0.0f);

  if (hDrcData == NULL) {
    return;
  }
  if (hDrcData->enable != 1) {
    return;
  }

  offset = offsetTab[frameLenFlag];

  useLP = (qmfImagSlot == NULL) ? 1 : 0;

  col += indx;
  bottomMdct = 0;
  bin = 0;

  /* get respective data and calc interpolation factor */
  if (col < (numQmfSubSamples>>1)) {  /* first half of current frame */
    if (hDrcData->winSequenceCurr != 2) { /* long window */
      int j = col + (numQmfSubSamples>>1);

      if (hDrcData->drcInterpolationSchemeCurr == 0) {
        INT k = (frameLenFlag) ? 0x4444444 : 0x4000000;

        alphaValue = (FIXP_DBL)(j * k);
      }
      else {
        if (j >= offset[hDrcData->drcInterpolationSchemeCurr - 1]) {
          alphaValue = FL2FXCONST_DBL(1.0f);
        }
      }
    }
    else {  /* short windows */
      shortDrc = 1;
    }

    fact_mag = hDrcData->currFact_mag;
    fact_exp = hDrcData->currFact_exp;
    numBands = hDrcData->numBandsCurr;
    bandTop = hDrcData->bandTopCurr;
  }
  else if (col < numQmfSubSamples) {  /* second half of current frame */
    if (hDrcData->winSequenceNext != 2) { /* next: long window */
      int j = col - (numQmfSubSamples>>1);

      if (hDrcData->drcInterpolationSchemeNext == 0) {
        INT k = (frameLenFlag) ? 0x4444444 : 0x4000000;

        alphaValue = (FIXP_DBL)(j * k);
      }
      else {
        if (j >= offset[hDrcData->drcInterpolationSchemeNext - 1]) {
          alphaValue = FL2FXCONST_DBL(1.0f);
        }
      }

      fact_mag = hDrcData->nextFact_mag;
      fact_exp = hDrcData->nextFact_exp;
      numBands = hDrcData->numBandsNext;
      bandTop = hDrcData->bandTopNext;
    }
    else {  /* next: short windows */
      if (hDrcData->winSequenceCurr != 2) {  /* current: long window */
        alphaValue = (FIXP_DBL)0;

        fact_mag = hDrcData->nextFact_mag;
        fact_exp = hDrcData->nextFact_exp;
        numBands = hDrcData->numBandsNext;
        bandTop = hDrcData->bandTopNext;
      }
      else {  /* current: short windows */
        shortDrc = 1;

        fact_mag = hDrcData->currFact_mag;
        fact_exp = hDrcData->currFact_exp;
        numBands = hDrcData->numBandsCurr;
        bandTop = hDrcData->bandTopCurr;
      }
    }
  }
  else {  /* first half of next frame */
    if (hDrcData->winSequenceNext != 2) { /* long window */
      int j = col - (numQmfSubSamples>>1);

      if (hDrcData->drcInterpolationSchemeNext == 0) {
        INT k = (frameLenFlag) ? 0x4444444 : 0x4000000;

        alphaValue = (FIXP_DBL)(j * k);
      }
      else {
        if (j >= offset[hDrcData->drcInterpolationSchemeNext - 1]) {
          alphaValue = FL2FXCONST_DBL(1.0f);
        }
      }
    }
    else {  /* short windows */
      shortDrc = 1;
    }

    fact_mag = hDrcData->nextFact_mag;
    fact_exp = hDrcData->nextFact_exp;
    numBands = hDrcData->numBandsNext;
    bandTop = hDrcData->bandTopNext;

    col -= numQmfSubSamples;
  }


  /* process bands */
  for (band = 0; band < (int)numBands; band++) {
    int bottomQmf, topQmf;

    FIXP_DBL drcFact_mag = FL2FXCONST_DBL(1.0f);

    topMdct = (bandTop[band]+1) << 2;

    if (!shortDrc) {  /* long window */
      if (frameLenFlag) {
        /* 960 framing */
        bottomMdct = 30 * (bottomMdct / 30);
        topMdct    = 30 * (topMdct / 30);

        bottomQmf = fMultIfloor((FIXP_DBL)0x4444444, bottomMdct);
        topQmf    = fMultIfloor((FIXP_DBL)0x4444444, topMdct);
      }
      else {
        /* 1024 framing */
        bottomMdct &= ~0x1f;
        topMdct    &= ~0x1f;

        bottomQmf = bottomMdct >> 5;
        topQmf    = topMdct >> 5;
      }

      if (band == ((int)numBands-1)) {
        topQmf = (64);
      }
    
      for (bin = bottomQmf; bin < topQmf; bin++) {
        FIXP_DBL drcFact1_mag = hDrcData->prevFact_mag[bin];
        FIXP_DBL drcFact2_mag = fact_mag[band];

        /* normalize scale factors */
        if (hDrcData->prevFact_exp < maxShift) {
          drcFact1_mag >>= maxShift - hDrcData->prevFact_exp;
        }
        if (fact_exp < maxShift) {
          drcFact2_mag >>= maxShift - fact_exp;
        }

        /* interpolate */
        drcFact_mag = fMult(alphaValue, drcFact2_mag) + fMult((FL2FXCONST_DBL(1.0f) - alphaValue), drcFact1_mag);

        /* apply scaling */
        qmfRealSlot[bin] = fMult(qmfRealSlot[bin], drcFact_mag);
        if (!useLP) {
          qmfImagSlot[bin] = fMult(qmfImagSlot[bin], drcFact_mag);
        }

        /* save previous factors */
        if (col == (numQmfSubSamples>>1)-1) {
          hDrcData->prevFact_mag[bin] = fact_mag[band];
        }
      }
    }
    else {  /* short windows */
      int startSample, stopSample;
      FIXP_DBL invFrameSizeDiv8 = (frameLenFlag) ? (FIXP_DBL)0x1111111 : (FIXP_DBL)0x1000000;

      if (frameLenFlag) {
        /*  960 framing */
        bottomMdct = 30/8 * (bottomMdct*8/30);
        topMdct    = 30/8 * (topMdct*8/30);
      }
      else {
        /* 1024 framing */
        bottomMdct &= ~0x03;
        topMdct    &= ~0x03;
      }

      /* startSample is truncated to the nearest corresponding start subsample in
         the QMF of the short window bottom is present in:*/
      startSample  = ((fMultIfloor( invFrameSizeDiv8, bottomMdct ) & 0x7) * numQmfSubSamples) >> 3;

      /* stopSample is rounded upwards to the nearest corresponding stop subsample
         in the QMF of the short window top is present in. */
      stopSample  = ((fMultIceil( invFrameSizeDiv8, topMdct ) & 0xf) * numQmfSubSamples) >> 3;

      bottomQmf = fMultIfloor( invFrameSizeDiv8, ((bottomMdct%(numQmfSubSamples<<2)) << 5) );
      topQmf    = fMultIfloor( invFrameSizeDiv8, ((topMdct%(numQmfSubSamples<<2)) << 5) );

      /* extend last band */
      if (band == ((int)numBands-1)) {
        topQmf = (64);
        stopSample = numQmfSubSamples;
      }

      if (topQmf == 0) {
        topQmf = (64);
      }

      /* save previous factors */
      if (stopSample == numQmfSubSamples) {
        int tmpBottom = bottomQmf;

        if (((numQmfSubSamples-1) & ~0x03) > startSample) {
            tmpBottom = 0;    /* band starts in previous short window */
        }

        for (bin = tmpBottom; bin < topQmf; bin++) {
          hDrcData->prevFact_mag[bin] = fact_mag[band];
        }
      }

      /* apply */
      if ((col >= startSample) && (col < stopSample)) {
        if ((col & ~0x03) > startSample) {
            bottomQmf = 0;    /* band starts in previous short window */
        }
        if (col < ((stopSample-1) & ~0x03)) {
            topQmf = (64);   /* band ends in next short window */
        }

        drcFact_mag = fact_mag[band];

        /* normalize scale factor */
        if (fact_exp < maxShift) {
          drcFact_mag >>= maxShift - fact_exp;
        }

        /* apply scaling */
        for (bin = bottomQmf; bin < topQmf; bin++) {
          qmfRealSlot[bin] = fMult(qmfRealSlot[bin], drcFact_mag);
          if (!useLP) {
            qmfImagSlot[bin] = fMult(qmfImagSlot[bin], drcFact_mag);
          }
        }
      }
    }

    bottomMdct = topMdct;
  }   /* end of bands loop */

  if (col == (numQmfSubSamples>>1)-1) {
    hDrcData->prevFact_exp = fact_exp;
  }
}


/*!
  \brief Apply DRC factors frame based.

  \hDrcData Handle to DRC channel data.
  \qmfRealSlot Pointer to real valued QMF data of the whole frame.
  \qmfImagSlot Pointer to the imaginary QMF data of the whole frame.
  \numQmfSubSamples Total number of time slots for one frame.
  \scaleFactor Pointer to the out scale factor of the frame.

  \return None.
*/
void sbrDecoder_drcApply (
    HANDLE_SBR_DRC_CHANNEL  hDrcData,
    FIXP_DBL **QmfBufferReal,
    FIXP_DBL **QmfBufferImag,
    int  numQmfSubSamples,
    int *scaleFactor
  )
{
  int col;
  int maxShift = 0;

  /* get max scale factor */
  if (hDrcData->prevFact_exp > maxShift) {
    maxShift = hDrcData->prevFact_exp;
  }
  if (hDrcData->currFact_exp > maxShift) {
    maxShift = hDrcData->currFact_exp;
  }
  if (hDrcData->nextFact_exp > maxShift) {
    maxShift = hDrcData->nextFact_exp;
  }

  for (col = 0; col < numQmfSubSamples; col++)
  {
    FIXP_DBL *qmfSlotReal = QmfBufferReal[col];
    FIXP_DBL *qmfSlotImag = (QmfBufferImag == NULL) ? NULL : QmfBufferImag[col];

    sbrDecoder_drcApplySlot (
      hDrcData,
      qmfSlotReal,
      qmfSlotImag,
      col,
      numQmfSubSamples,
      maxShift
    );
  }

  *scaleFactor += maxShift;
}

