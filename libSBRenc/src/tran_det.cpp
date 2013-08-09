
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2013 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

#include "tran_det.h"

#include "fram_gen.h"
#include "sbr_ram.h"
#include "sbr_misc.h"

#include "genericStds.h"

#define NORM_QMF_ENERGY 5.684341886080801486968994140625e-14 /* 2^-44 */

/* static FIXP_DBL ABS_THRES = fixMax( FL2FXCONST_DBL(1.28e5 *  NORM_QMF_ENERGY), (FIXP_DBL)1)  Minimum threshold for detecting changes */
#define ABS_THRES ((FIXP_DBL)16)

/*******************************************************************************
 Functionname:  spectralChange
 *******************************************************************************
 \brief   Calculates a measure for the spectral change within the frame

 The function says how good it would be to split the frame at the given border
 position into 2 envelopes.

 The return value delta_sum is scaled with the factor 1/64

 \return  calculated value
*******************************************************************************/
static FIXP_DBL spectralChange(FIXP_DBL Energies[NUMBER_TIME_SLOTS_2304][MAX_FREQ_COEFFS],
                               INT *scaleEnergies,
                               FIXP_DBL EnergyTotal,
                               INT nSfb,
                               INT start,
                               INT border,
                               INT stop)
{
  INT i,j;
  INT len1,len2;
  FIXP_DBL delta,tmp0,tmp1,tmp2;
  FIXP_DBL accu1,accu2,delta_sum,result;

  FDK_ASSERT(scaleEnergies[0] >= 0);

  /* equal for aac (would be not equal for mp3) */
  len1 = border-start;
  len2 = stop-border;

  /* prefer borders near the middle of the frame */
  FIXP_DBL   pos_weight;
  pos_weight = FL2FXCONST_DBL(0.5f) - (len1*GetInvInt(len1+len2));
  pos_weight = /*FL2FXCONST_DBL(1.0)*/ (FIXP_DBL)MAXVAL_DBL - (fMult(pos_weight, pos_weight)<<2);

  delta_sum = FL2FXCONST_DBL(0.0f);

  /* Sum up energies of all QMF-timeslots for both halfs */
  for (j=0; j<nSfb; j++) {
    #define NRG_SCALE  3
    /* init with some energy to prevent division by zero
       and to prevent splitting for very low levels */
    accu1 = ((FL2FXCONST_DBL((1.0e6*NORM_QMF_ENERGY*8.0/32))) << fixMin(scaleEnergies[0],25))>>NRG_SCALE;  /* complex init for compare with original version */
    accu2 = ((FL2FXCONST_DBL((1.0e6*NORM_QMF_ENERGY*8.0/32))) << fixMin(scaleEnergies[0],25))>>NRG_SCALE;  /* can be simplified in dsp implementation */

    /* Sum up energies in first half */
    for (i=start; i<border; i++) {
      accu1 += (Energies[i][j]>>NRG_SCALE);
    }

    /* Sum up energies in second half */
    for (i=border; i<stop; i++) {
      accu2 += (Energies[i][j]>>NRG_SCALE);
    }

    /* Energy change in current band */
    tmp0 = CalcLdData(accu2);
    tmp1 = CalcLdData(accu1);
    tmp2 = (tmp0 - tmp1 + CalcLdData(len1)-CalcLdData(len2));
    delta = fixp_abs(fMult(tmp2, FL2FXCONST_DBL(0.6931471806f)));

    /* Weighting with amplitude ratio of this band */
    result = (EnergyTotal == FL2FXCONST_DBL(0.0f))
            ? FL2FXCONST_DBL(0.f)
            : FDKsbrEnc_LSI_divide_scale_fract( (accu1+accu2),
                                      (EnergyTotal>>NRG_SCALE)+(FIXP_DBL)1,
                                      (FIXP_DBL)MAXVAL_DBL >> fixMin(scaleEnergies[0],(DFRACT_BITS-1)) );

    delta_sum += (FIXP_DBL)(fMult(sqrtFixp(result), delta));
  }

  return fMult(delta_sum, pos_weight);
}


/*******************************************************************************
 Functionname:  addLowbandEnergies
 *******************************************************************************
 \brief   Calculates total lowband energy

 The return value nrgTotal is scaled by the factor (1/32.0)

 \return  total energy in the lowband
*******************************************************************************/
static FIXP_DBL addLowbandEnergies(FIXP_DBL **Energies,
                                   int       *scaleEnergies,
                                   int        YBufferWriteOffset,
                                   int        nrgSzShift,
                                   int        tran_off,
                                   UCHAR     *freqBandTable,
                                   int        slots)
{
  FIXP_DBL nrgTotal;
  FIXP_DBL accu1 = FL2FXCONST_DBL(0.0f);
  FIXP_DBL accu2 = FL2FXCONST_DBL(0.0f);
  int tran_offdiv2 = tran_off>>nrgSzShift;
  int ts,k;

  /* Sum up lowband energy from one frame at offset tran_off */
  for (ts=tran_offdiv2; ts<YBufferWriteOffset; ts++) {
    for (k = 0; k < freqBandTable[0]; k++) {
      accu1 += Energies[ts][k] >> 6;
    }
  }
  for (; ts<tran_offdiv2+(slots>>nrgSzShift); ts++) {
    for (k = 0; k < freqBandTable[0]; k++) {
      accu2 += Energies[ts][k] >> 6;
    }
  }

  nrgTotal = ( (accu1 >> fixMin(scaleEnergies[0],(DFRACT_BITS-1)))
           +   (accu2 >> fixMin(scaleEnergies[1],(DFRACT_BITS-1))) ) << (2);

  return(nrgTotal);
}


/*******************************************************************************
 Functionname:  addHighbandEnergies
 *******************************************************************************
 \brief   Add highband energies

 Highband energies are mapped to an array with smaller dimension:
 Its time resolution is only 1 SBR-timeslot and its frequency resolution
 is 1 SBR-band. Therefore the data to be fed into the spectralChange
 function is reduced.

 The values EnergiesM are scaled by the factor (1/32.0) and scaleEnergies[0]
 The return value nrgTotal is scaled by the factor (1/32.0)

 \return  total energy in the highband
*******************************************************************************/

static FIXP_DBL addHighbandEnergies(FIXP_DBL **RESTRICT Energies, /*!< input */
                                    INT       *scaleEnergies,
                                    FIXP_DBL   EnergiesM[NUMBER_TIME_SLOTS_2304][MAX_FREQ_COEFFS], /*!< Combined output */
                                    UCHAR     *RESTRICT freqBandTable,
                                    INT        nSfb,
                                    INT        sbrSlots,
                                    INT        timeStep)
{
  INT i,j,k,slotIn,slotOut,scale;
  INT li,ui;
  FIXP_DBL nrgTotal;
  FIXP_DBL accu = FL2FXCONST_DBL(0.0f);

  /* Combine QMF-timeslots to SBR-timeslots,
     combine QMF-bands to SBR-bands,
     combine Left and Right channel */
  for (slotOut=0; slotOut<sbrSlots; slotOut++) {
    slotIn = 2*slotOut;

    for (j=0; j<nSfb; j++) {
      accu = FL2FXCONST_DBL(0.0f);

      li = freqBandTable[j];
      ui = freqBandTable[j + 1];

      for (k=li; k<ui; k++) {
        for (i=0; i<timeStep; i++) {
         accu += (Energies[(slotIn+i)>>1][k] >> 5);
        }
      }
      EnergiesM[slotOut][j] = accu;
    }
  }

  scale = fixMin(8,scaleEnergies[0]);      /* scale energies down before add up */

  if ((scaleEnergies[0]-1) > (DFRACT_BITS-1) )
    nrgTotal = FL2FXCONST_DBL(0.0f);
  else {
    /* Now add all energies */
    accu = FL2FXCONST_DBL(0.0f);
    for (slotOut=0; slotOut<sbrSlots; slotOut++) {
      for (j=0; j<nSfb; j++) {
        accu += (EnergiesM[slotOut][j] >> scale);
      }
    }
    nrgTotal = accu >> (scaleEnergies[0]-scale);
  }

  return(nrgTotal);
}


/*******************************************************************************
 Functionname:  FDKsbrEnc_frameSplitter
 *******************************************************************************
 \brief   Decides if a FIXFIX-frame shall be splitted into 2 envelopes

 If no transient has been detected before, the frame can still be splitted
 into 2 envelopes.
*******************************************************************************/
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
                        int no_cols)
{
  if (tran_vector[1]==0) /* no transient was detected */
  {
    FIXP_DBL delta;
    FIXP_DBL EnergiesM[NUMBER_TIME_SLOTS_2304][MAX_FREQ_COEFFS];
    FIXP_DBL EnergyTotal,newLowbandEnergy,newHighbandEnergy;
    INT border;
    INT sbrSlots = fMultI(GetInvInt(timeStep),no_cols);

    FDK_ASSERT( sbrSlots * timeStep == no_cols );

    /*
      Get Lowband-energy over a range of 2 frames (Look half a frame back and ahead).
    */
    newLowbandEnergy = addLowbandEnergies(Energies,
                                          scaleEnergies,
                                          YBufferWriteOffset,
                                          YBufferSzShift,
                                          h_sbrTransientDetector->tran_off,
                                          freqBandTable,
                                          no_cols);

    newHighbandEnergy = addHighbandEnergies(Energies,
                                            scaleEnergies,
                                            EnergiesM,
                                            freqBandTable,
                                            nSfb,
                                            sbrSlots,
                                            timeStep);

    if ( h_sbrTransientDetector->frameShift != 0 ) {
      if (tran_vector[1]==0)
        tran_vector[0] = 0;
    } else
    {
      /* prevLowBandEnergy: Corresponds to 1 frame, starting with half a frame look-behind
         newLowbandEnergy:  Corresponds to 1 frame, starting in the middle of the current frame */
      EnergyTotal = (newLowbandEnergy + h_sbrTransientDetector->prevLowBandEnergy) >> 1;
      EnergyTotal += newHighbandEnergy;
      /* The below border should specify the same position as the middle border
         of a FIXFIX-frame with 2 envelopes. */
      border = (sbrSlots+1) >> 1;

      delta = spectralChange(EnergiesM,
                             scaleEnergies,
                             EnergyTotal,
                             nSfb,
                             0,
                             border,
                             sbrSlots);

      if (delta > (h_sbrTransientDetector->split_thr >> LD_DATA_SHIFT)) /* delta scaled by 1/64 */
        tran_vector[0] = 1; /* Set flag for splitting */
      else
        tran_vector[0] = 0;
    }

    /* Update prevLowBandEnergy */
    h_sbrTransientDetector->prevLowBandEnergy = newLowbandEnergy;
    h_sbrTransientDetector->prevHighBandEnergy = newHighbandEnergy;
  }
}

/*
 * Calculate transient energy threshold for each QMF band
 */
static void
calculateThresholds(FIXP_DBL **RESTRICT Energies,
                    INT       *RESTRICT scaleEnergies,
                    FIXP_DBL  *RESTRICT thresholds,
                    int        YBufferWriteOffset,
                    int        YBufferSzShift,
                    int        noCols,
                    int        noRows,
                    int        tran_off)
{
  FIXP_DBL mean_val,std_val,temp;
  FIXP_DBL i_noCols;
  FIXP_DBL i_noCols1;
  FIXP_DBL accu,accu0,accu1;
  int scaleFactor0,scaleFactor1,commonScale;
  int i,j;

  i_noCols  = GetInvInt(noCols + tran_off ) << YBufferSzShift;
  i_noCols1 = GetInvInt(noCols + tran_off - 1) << YBufferSzShift;

  /* calc minimum scale of energies of previous and current frame */
  commonScale = fixMin(scaleEnergies[0],scaleEnergies[1]);

  /* calc scalefactors to adapt energies to common scale */
  scaleFactor0 = fixMin((scaleEnergies[0]-commonScale), (DFRACT_BITS-1));
  scaleFactor1 = fixMin((scaleEnergies[1]-commonScale), (DFRACT_BITS-1));

  FDK_ASSERT((scaleFactor0 >= 0) && (scaleFactor1 >= 0));

  /* calculate standard deviation in every subband */
  for (i=0; i<noRows; i++)
  {
    int startEnergy = (tran_off>>YBufferSzShift);
    int endEnergy = ((noCols>>YBufferSzShift)+tran_off);
    int shift;

    /* calculate mean value over decimated energy values (downsampled by 2). */
    accu0 = accu1 = FL2FXCONST_DBL(0.0f);

    for (j=startEnergy; j<YBufferWriteOffset; j++)
      accu0 += fMult(Energies[j][i], i_noCols);
    for (; j<endEnergy; j++)
      accu1 += fMult(Energies[j][i], i_noCols);

    mean_val = (accu0 >> scaleFactor0) + (accu1 >> scaleFactor1);  /* average */
    shift    = fixMax(0,CountLeadingBits(mean_val)-6);             /* -6 to keep room for accumulating upto N = 24 values */

    /* calculate standard deviation */
    accu = FL2FXCONST_DBL(0.0f);

    /* summe { ((mean_val-nrg)^2) * i_noCols1 } */
    for (j=startEnergy; j<YBufferWriteOffset; j++) {
      temp = ((FIXP_DBL)mean_val - ((FIXP_DBL)Energies[j][i] >> scaleFactor0))<<shift;
      temp = fPow2(temp);
      temp = fMult(temp, i_noCols1);
      accu += temp;
    }
    for (; j<endEnergy; j++) {
      temp = ((FIXP_DBL)mean_val - ((FIXP_DBL)Energies[j][i] >> scaleFactor1))<<shift;
      temp = fPow2(temp);
      temp = fMult(temp, i_noCols1);
      accu += temp;
    }

    std_val = sqrtFixp(accu)>>shift;     /* standard deviation */

    /*
    Take new threshold as average of calculated standard deviation ratio
    and old threshold if greater than absolute threshold
    */
    temp = ( commonScale<=(DFRACT_BITS-1) )
            ? fMult(FL2FXCONST_DBL(0.66f), thresholds[i]) + (fMult(FL2FXCONST_DBL(0.34f), std_val) >> commonScale)
            : (FIXP_DBL) 0;

    thresholds[i] = fixMax(ABS_THRES,temp);

    FDK_ASSERT(commonScale >= 0);
  }
}

/*
 * Calculate transient levels for each QMF time slot.
 */
static void
extractTransientCandidates(FIXP_DBL  **RESTRICT Energies,
                           INT        *RESTRICT scaleEnergies,
                           FIXP_DBL   *RESTRICT thresholds,
                           FIXP_DBL   *RESTRICT transients,
                           int         YBufferWriteOffset,
                           int         YBufferSzShift,
                           int         noCols,
                           int         start_band,
                           int         stop_band,
                           int         tran_off,
                           int         addPrevSamples)
{
  FIXP_DBL i_thres;
  C_ALLOC_SCRATCH_START(EnergiesTemp, FIXP_DBL, 2*QMF_MAX_TIME_SLOTS);
  FIXP_DBL *RESTRICT pEnergiesTemp = EnergiesTemp;
  int tmpScaleEnergies0, tmpScaleEnergies1;
  int endCond;
  int startEnerg,endEnerg;
  int i,j,jIndex,jpBM;

  tmpScaleEnergies0 = scaleEnergies[0];
  tmpScaleEnergies1 = scaleEnergies[1];

  /* Scale value for first energies, upto YBufferWriteOffset */
  tmpScaleEnergies0 = fixMin(tmpScaleEnergies0, MAX_SHIFT_DBL);
  /* Scale value for first energies, from YBufferWriteOffset upwards */
  tmpScaleEnergies1 = fixMin(tmpScaleEnergies1, MAX_SHIFT_DBL);

  FDK_ASSERT((tmpScaleEnergies0 >= 0) && (tmpScaleEnergies1 >= 0));

  /* Keep addPrevSamples extra previous transient candidates. */
  FDKmemmove(transients, transients + noCols - addPrevSamples, (tran_off+addPrevSamples) * sizeof (FIXP_DBL));
  FDKmemclear(transients + tran_off + addPrevSamples, noCols * sizeof (FIXP_DBL));

  endCond = noCols; /* Amount of new transient values to be calculated. */
  startEnerg = (tran_off-3)>>YBufferSzShift; /* >>YBufferSzShift because of amount of energy values. -3 because of neighbors being watched. */
  endEnerg = ((noCols+ (YBufferWriteOffset<<YBufferSzShift))-1)>>YBufferSzShift; /* YBufferSzShift shifts because of half energy values. */

  /* Compute differential values with two different weightings in every subband */
  for (i=start_band; i<stop_band; i++)
  {
    FIXP_DBL thres = thresholds[i];

    if((LONG)thresholds[i]>=256)
      i_thres = (LONG)( (LONG)MAXVAL_DBL / ((((LONG)thresholds[i]))+1) )<<(32-24);
    else
      i_thres = (LONG)MAXVAL_DBL;

    /* Copy one timeslot and de-scale and de-squish */
    if (YBufferSzShift == 1) {
      for(j=startEnerg; j<YBufferWriteOffset; j++) {
        FIXP_DBL tmp = Energies[j][i];
        EnergiesTemp[(j<<1)+1] = EnergiesTemp[j<<1] = tmp>>tmpScaleEnergies0;
      }
      for(; j<=endEnerg; j++) {
        FIXP_DBL tmp = Energies[j][i];
        EnergiesTemp[(j<<1)+1] = EnergiesTemp[j<<1] = tmp>>tmpScaleEnergies1;
      }
    } else {
      for(j=startEnerg; j<YBufferWriteOffset; j++) {
        FIXP_DBL tmp = Energies[j][i];
        EnergiesTemp[j] = tmp>>tmpScaleEnergies0;
      }
      for(; j<=endEnerg; j++) {
        FIXP_DBL tmp = Energies[j][i];
        EnergiesTemp[j] = tmp>>tmpScaleEnergies1;
      }
    }

    /* Detect peaks in energy values. */

    jIndex = tran_off;
    jpBM = jIndex+addPrevSamples;

    for (j=endCond; j--; jIndex++, jpBM++)
    {

      FIXP_DBL delta, tran;
      int d;

      delta = (FIXP_DBL)0;
      tran  = (FIXP_DBL)0;

      for (d=1; d<4; d++) {
        delta += pEnergiesTemp[jIndex+d]; /* R */
        delta -= pEnergiesTemp[jIndex-d]; /* L */
        delta -= thres;

        if ( delta > (FIXP_DBL)0 ) {
          tran += fMult(i_thres, delta);
        }
      }
      transients[jpBM] += tran;
    }
  }
  C_ALLOC_SCRATCH_END(EnergiesTemp, FIXP_DBL, 2*QMF_MAX_TIME_SLOTS);
}

void
FDKsbrEnc_transientDetect(HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTran,
                          FIXP_DBL **Energies,
                          INT *scaleEnergies,
                          UCHAR *transient_info,
                          int YBufferWriteOffset,
                          int YBufferSzShift,
                          int timeStep,
                          int frameMiddleBorder)
{
  int no_cols = h_sbrTran->no_cols;
  int qmfStartSample;
  int addPrevSamples;
  int timeStepShift=0;
  int i, cond;

  /* Where to start looking for transients in the transient candidate buffer */
  qmfStartSample = timeStep * frameMiddleBorder;
  /* We need to look one value backwards in the transients, so we might need one more previous value. */
  addPrevSamples = (qmfStartSample > 0) ? 0: 1;

  switch (timeStep) {
    case 1: timeStepShift = 0; break;
    case 2: timeStepShift = 1; break;
    case 4: timeStepShift = 2; break;
  }

  calculateThresholds(Energies,
                      scaleEnergies,
                      h_sbrTran->thresholds,
                      YBufferWriteOffset,
                      YBufferSzShift,
                      h_sbrTran->no_cols,
                      h_sbrTran->no_rows,
                      h_sbrTran->tran_off);

  extractTransientCandidates(Energies,
                             scaleEnergies,
                             h_sbrTran->thresholds,
                             h_sbrTran->transients,
                             YBufferWriteOffset,
                             YBufferSzShift,
                             h_sbrTran->no_cols,
                             0,
                             h_sbrTran->no_rows,
                             h_sbrTran->tran_off,
                             addPrevSamples );

  transient_info[0] = 0;
  transient_info[1] = 0;
  transient_info[2] = 0;

  /* Offset by the amount of additional previous transient candidates being kept. */
  qmfStartSample += addPrevSamples;

  /* Check for transients in second granule (pick the last value of subsequent values)  */
  for (i=qmfStartSample; i<qmfStartSample + no_cols; i++) {
    cond =    (h_sbrTran->transients[i] < fMult(FL2FXCONST_DBL(0.9f), h_sbrTran->transients[i - 1]) )
           && (h_sbrTran->transients[i - 1] > h_sbrTran->tran_thr);

    if (cond) {
      transient_info[0] = (i - qmfStartSample)>>timeStepShift;
      transient_info[1] = 1;
      break;
    }
  }

  if ( h_sbrTran->frameShift != 0) {
      /* transient prediction for LDSBR */
      /* Check for transients in first <frameShift> qmf-slots of second frame */
      for (i=qmfStartSample+no_cols; i<qmfStartSample + no_cols+h_sbrTran->frameShift; i++) {

        cond =    (h_sbrTran->transients[i] < fMult(FL2FXCONST_DBL(0.9f), h_sbrTran->transients[i - 1]) )
               && (h_sbrTran->transients[i - 1] > h_sbrTran->tran_thr);

        if (cond) {
          int pos = (int) ( (i - qmfStartSample-no_cols) >> timeStepShift );
          if ((pos < 3) && (transient_info[1]==0)) {
            transient_info[2] = 1;
          }
          break;
        }
      }
  }
}

int
FDKsbrEnc_InitSbrTransientDetector(HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,
                                   INT   frameSize,
                                   INT   sampleFreq,
                                   sbrConfigurationPtr params,
                                   int   tran_fc,
                                   int   no_cols,
                                   int   no_rows,
                                   int   YBufferWriteOffset,
                                   int   YBufferSzShift,
                                   int   frameShift,
                                   int   tran_off)
{
    INT totalBitrate = params->codecSettings.standardBitrate * params->codecSettings.nChannels;
    INT codecBitrate = params->codecSettings.bitRate;
    FIXP_DBL bitrateFactor_fix, framedur_fix;
    INT scale_0, scale_1;

    FDKmemclear(h_sbrTransientDetector,sizeof(SBR_TRANSIENT_DETECTOR));

    h_sbrTransientDetector->frameShift = frameShift;
    h_sbrTransientDetector->tran_off = tran_off;

    if(codecBitrate) {
      bitrateFactor_fix = fDivNorm((FIXP_DBL)totalBitrate, (FIXP_DBL)(codecBitrate<<2),&scale_0);
    }
    else {
      bitrateFactor_fix = FL2FXCONST_DBL(1.0/4.0);
      scale_0 = 0;
    }

    framedur_fix = fDivNorm(frameSize, sampleFreq);

    /* The longer the frames, the more often should the FIXFIX-
    case transmit 2 envelopes instead of 1.
    Frame durations below 10 ms produce the highest threshold
    so that practically always only 1 env is transmitted. */
    FIXP_DBL tmp = framedur_fix - FL2FXCONST_DBL(0.010);

    tmp = fixMax(tmp, FL2FXCONST_DBL(0.0001));
    tmp = fDivNorm(FL2FXCONST_DBL(0.000075), fPow2(tmp), &scale_1);

    scale_1 = -(scale_1 + scale_0 + 2);

    FDK_ASSERT(no_cols <= QMF_MAX_TIME_SLOTS);
    FDK_ASSERT(no_rows <= QMF_CHANNELS);

    h_sbrTransientDetector->no_cols = no_cols;
    h_sbrTransientDetector->tran_thr = (FIXP_DBL)((params->tran_thr << (32-24-1)) / no_rows);
    h_sbrTransientDetector->tran_fc = tran_fc;

    if (scale_1>=0) {
      h_sbrTransientDetector->split_thr = fMult(tmp, bitrateFactor_fix) >> scale_1;
    }
    else {
      h_sbrTransientDetector->split_thr = fMult(tmp, bitrateFactor_fix) << (-scale_1);
    }

    h_sbrTransientDetector->no_rows = no_rows;
    h_sbrTransientDetector->mode = params->tran_det_mode;
    h_sbrTransientDetector->prevLowBandEnergy = FL2FXCONST_DBL(0.0f);

    return (0);
}

