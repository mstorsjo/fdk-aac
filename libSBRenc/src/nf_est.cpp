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

#include "nf_est.h"

#include "sbr_misc.h"

#include "genericStds.h"

/* smoothFilter[4]  = {0.05857864376269f, 0.2f, 0.34142135623731f, 0.4f}; */
static const FIXP_DBL smoothFilter[4]  = { 0x077f813d, 0x19999995, 0x2bb3b1f5, 0x33333335 };

/* static const INT smoothFilterLength = 4; */

static const FIXP_DBL QuantOffset = (INT)0xfc000000;  /* ld64(0.25) */

#ifndef min
#define min(a,b) ( a < b ? a:b)
#endif

#ifndef max
#define max(a,b) ( a > b ? a:b)
#endif

#define NOISE_FLOOR_OFFSET_SCALING  (3)



/**************************************************************************/
/*!
  \brief     The function applies smoothing to the noise levels.



  \return    none

*/
/**************************************************************************/
static void
smoothingOfNoiseLevels(FIXP_DBL *NoiseLevels,        /*!< pointer to noise-floor levels.*/
                       INT nEnvelopes,               /*!< Number of noise floor envelopes.*/
                       INT noNoiseBands,             /*!< Number of noise bands for every noise floor envelope. */
                       FIXP_DBL prevNoiseLevels[NF_SMOOTHING_LENGTH][MAX_NUM_NOISE_VALUES],/*!< Previous noise floor envelopes. */
                       const FIXP_DBL *smoothFilter, /*!< filter used for smoothing the noise floor levels. */
                       INT transientFlag)            /*!< flag indicating if a transient is present*/

{
  INT i,band,env;
  FIXP_DBL accu;

  for(env = 0; env < nEnvelopes; env++){
    if(transientFlag){
      for (i = 0; i < NF_SMOOTHING_LENGTH; i++){
        FDKmemcpy(prevNoiseLevels[i],NoiseLevels+env*noNoiseBands,noNoiseBands*sizeof(FIXP_DBL));
      }
    }
    else {
      for (i = 1; i < NF_SMOOTHING_LENGTH; i++){
        FDKmemcpy(prevNoiseLevels[i - 1],prevNoiseLevels[i],noNoiseBands*sizeof(FIXP_DBL));
      }
      FDKmemcpy(prevNoiseLevels[NF_SMOOTHING_LENGTH - 1],NoiseLevels+env*noNoiseBands,noNoiseBands*sizeof(FIXP_DBL));
    }

    for (band = 0; band < noNoiseBands; band++){
      accu = FL2FXCONST_DBL(0.0f);
      for (i = 0; i < NF_SMOOTHING_LENGTH; i++){
        accu += fMultDiv2(smoothFilter[i], prevNoiseLevels[i][band]);
      }
      FDK_ASSERT( (band + env*noNoiseBands) < MAX_NUM_NOISE_VALUES);
      NoiseLevels[band+ env*noNoiseBands] = accu<<1;
    }
  }
}

/**************************************************************************/
/*!
  \brief     Does the noise floor level estiamtion.

  The noiseLevel samples are scaled by the factor 0.25

  \return    none

*/
/**************************************************************************/
static void
qmfBasedNoiseFloorDetection(FIXP_DBL *noiseLevel,              /*!< Pointer to vector to store the noise levels in.*/
                            FIXP_DBL ** quotaMatrixOrig,       /*!< Matrix holding the quota values of the original. */
                            SCHAR *indexVector,                /*!< Index vector to obtain the patched data. */
                            INT startIndex,                    /*!< Start index. */
                            INT stopIndex,                     /*!< Stop index. */
                            INT startChannel,                  /*!< Start channel of the current noise floor band.*/
                            INT stopChannel,                   /*!< Stop channel of the current noise floor band. */
                            FIXP_DBL ana_max_level,            /*!< Maximum level of the adaptive noise.*/
                            FIXP_DBL noiseFloorOffset,         /*!< Noise floor offset. */
                            INT missingHarmonicFlag,           /*!< Flag indicating if a strong tonal component is missing.*/
                            FIXP_DBL weightFac,                /*!< Weightening factor for the difference between orig and sbr. */
                            INVF_MODE diffThres,               /*!< Threshold value to control the inverse filtering decision.*/
                            INVF_MODE inverseFilteringLevel)   /*!< Inverse filtering level of the current band.*/
{
  INT scale, l, k;
  FIXP_DBL meanOrig=FL2FXCONST_DBL(0.0f), meanSbr=FL2FXCONST_DBL(0.0f), diff;
  FIXP_DBL invIndex = GetInvInt(stopIndex-startIndex);
  FIXP_DBL invChannel = GetInvInt(stopChannel-startChannel);
  FIXP_DBL accu;

   /*
   Calculate the mean value, over the current time segment, for the original, the HFR
   and the difference, over all channels in the current frequency range.
   */

  if(missingHarmonicFlag == 1){
    for(l = startChannel; l < stopChannel;l++){
      /* tonalityOrig */
      accu = FL2FXCONST_DBL(0.0f);
      for(k = startIndex ; k < stopIndex; k++){
        accu += fMultDiv2(quotaMatrixOrig[k][l], invIndex);
      }
      meanOrig = fixMax(meanOrig,(accu<<1));

      /* tonalitySbr */
      accu = FL2FXCONST_DBL(0.0f);
      for(k = startIndex ; k < stopIndex; k++){
        accu += fMultDiv2(quotaMatrixOrig[k][indexVector[l]], invIndex);
      }
      meanSbr  = fixMax(meanSbr,(accu<<1));

    }
  }
  else{
    for(l = startChannel; l < stopChannel;l++){
      /* tonalityOrig */
      accu = FL2FXCONST_DBL(0.0f);
      for(k = startIndex ; k < stopIndex; k++){
        accu += fMultDiv2(quotaMatrixOrig[k][l], invIndex);
      }
      meanOrig += fMult((accu<<1), invChannel);

      /* tonalitySbr */
      accu = FL2FXCONST_DBL(0.0f);
      for(k = startIndex ; k < stopIndex; k++){
        accu += fMultDiv2(quotaMatrixOrig[k][indexVector[l]], invIndex);
      }
      meanSbr  += fMult((accu<<1), invChannel);
    }
  }

  /* Small fix to avoid noise during silent passages.*/
  if( meanOrig <= FL2FXCONST_DBL(0.000976562f*RELAXATION_FLOAT) &&
      meanSbr <= FL2FXCONST_DBL(0.000976562f*RELAXATION_FLOAT) )
  {
    meanOrig = FL2FXCONST_DBL(101.5936673f*RELAXATION_FLOAT);
    meanSbr  = FL2FXCONST_DBL(101.5936673f*RELAXATION_FLOAT);
  }

  meanOrig = fixMax(meanOrig,RELAXATION);
  meanSbr  = fixMax(meanSbr,RELAXATION);

  if (missingHarmonicFlag == 1 ||
      inverseFilteringLevel == INVF_MID_LEVEL ||
      inverseFilteringLevel == INVF_LOW_LEVEL ||
      inverseFilteringLevel == INVF_OFF ||
      inverseFilteringLevel <= diffThres)
  {
    diff = RELAXATION;
  }
  else {
    accu = fDivNorm(meanSbr, meanOrig, &scale);

    diff = fixMax( RELAXATION,
                   fMult(RELAXATION_FRACT,fMult(weightFac,accu)) >>( RELAXATION_SHIFT-scale ) ) ;
  }

  /*
   * noise Level is now a positive value, i.e.
   * the more harmonic the signal is the higher noise level,
   * this makes no sense so we change the sign.
   *********************************************************/
  accu = fDivNorm(diff, meanOrig, &scale);
  scale -= 2;

  if ( (scale>0) && (accu > ((FIXP_DBL)MAXVAL_DBL)>>scale) ) {
    *noiseLevel = (FIXP_DBL)MAXVAL_DBL;
  }
  else {
    *noiseLevel = scaleValue(accu, scale);
  }

  /*
   * Add a noise floor offset to compensate for bias in the detector
   *****************************************************************/
  if(!missingHarmonicFlag)
    *noiseLevel = fMult(*noiseLevel, noiseFloorOffset)<<(NOISE_FLOOR_OFFSET_SCALING);

  /*
   * check to see that we don't exceed the maximum allowed level
   **************************************************************/
  *noiseLevel = fixMin(*noiseLevel, ana_max_level);     /* ana_max_level is scaled with factor 0.25 */
}

/**************************************************************************/
/*!
  \brief     Does the noise floor level estiamtion.
  The function calls the Noisefloor estimation function
  for the time segments decided based upon the transient
  information. The block is always divided into one or two segments.


  \return    none

*/
/**************************************************************************/
void
FDKsbrEnc_sbrNoiseFloorEstimateQmf(HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate, /*!< Handle to SBR_NOISE_FLOOR_ESTIMATE struct */
                         const SBR_FRAME_INFO *frame_info,   /*!< Time frequency grid of the current frame. */
                         FIXP_DBL *noiseLevels,              /*!< Pointer to vector to store the noise levels in.*/
                         FIXP_DBL **quotaMatrixOrig,         /*!< Matrix holding the quota values of the original. */
                         SCHAR    *indexVector,              /*!< Index vector to obtain the patched data. */
                         INT missingHarmonicsFlag,           /*!< Flag indicating if a strong tonal component will be missing. */
                         INT startIndex,                     /*!< Start index. */
                         int numberOfEstimatesPerFrame,      /*!< The number of tonality estimates per frame. */
                         int transientFrame,                 /*!< A flag indicating if a transient is present. */
                         INVF_MODE* pInvFiltLevels,          /*!< Pointer to the vector holding the inverse filtering levels. */
                         UINT sbrSyntaxFlags
                         )

{

  INT nNoiseEnvelopes, startPos[2], stopPos[2], env, band;

  INT noNoiseBands      = h_sbrNoiseFloorEstimate->noNoiseBands;
  INT *freqBandTable    = h_sbrNoiseFloorEstimate->freqBandTableQmf;

  nNoiseEnvelopes = frame_info->nNoiseEnvelopes;

  if (sbrSyntaxFlags & SBR_SYNTAX_LOW_DELAY) {
    nNoiseEnvelopes = 1;
    startPos[0] = startIndex;
    stopPos[0]  = startIndex + min(numberOfEstimatesPerFrame,2);
  } else
  if(nNoiseEnvelopes == 1){
    startPos[0] = startIndex;
    stopPos[0]  = startIndex + 2;
  }
  else{
    startPos[0] = startIndex;
    stopPos[0]  = startIndex + 1;
    startPos[1] = startIndex + 1;
    stopPos[1]  = startIndex + 2;
  }

  /*
   * Estimate the noise floor.
   **************************************/
  for(env = 0; env < nNoiseEnvelopes; env++){
    for(band = 0; band < noNoiseBands; band++){
      FDK_ASSERT( (band + env*noNoiseBands) < MAX_NUM_NOISE_VALUES);
      qmfBasedNoiseFloorDetection(&noiseLevels[band + env*noNoiseBands],
                                  quotaMatrixOrig,
                                  indexVector,
                                  startPos[env],
                                  stopPos[env],
                                  freqBandTable[band],
                                  freqBandTable[band+1],
                                  h_sbrNoiseFloorEstimate->ana_max_level,
                                  h_sbrNoiseFloorEstimate->noiseFloorOffset[band],
                                  missingHarmonicsFlag,
                                  h_sbrNoiseFloorEstimate->weightFac,
                                  h_sbrNoiseFloorEstimate->diffThres,
                                  pInvFiltLevels[band]);
    }
  }


  /*
   * Smoothing of the values.
   **************************/
  smoothingOfNoiseLevels(noiseLevels,
                         nNoiseEnvelopes,
                         h_sbrNoiseFloorEstimate->noNoiseBands,
                         h_sbrNoiseFloorEstimate->prevNoiseLevels,
                         h_sbrNoiseFloorEstimate->smoothFilter,
                         transientFrame);


  /* quantisation*/
  for(env = 0; env < nNoiseEnvelopes; env++){
    for(band = 0; band < noNoiseBands; band++){
      FDK_ASSERT( (band + env*noNoiseBands) < MAX_NUM_NOISE_VALUES);
      noiseLevels[band + env*noNoiseBands] =
         (FIXP_DBL)NOISE_FLOOR_OFFSET_64 - (FIXP_DBL)CalcLdData(noiseLevels[band + env*noNoiseBands]+(FIXP_DBL)1) + QuantOffset;
    }
  }
}

/**************************************************************************/
/*!
  \brief


  \return    errorCode, noError if successful

*/
/**************************************************************************/
static INT
downSampleLoRes(INT *v_result,              /*!<    */
                INT num_result,             /*!<    */
                const UCHAR *freqBandTableRef,/*!<    */
                INT num_Ref)                /*!<    */
{
  INT step;
  INT i,j;
  INT org_length,result_length;
  INT v_index[MAX_FREQ_COEFFS/2];

  /* init */
  org_length=num_Ref;
  result_length=num_result;

  v_index[0]=0;	/* Always use left border */
  i=0;
  while(org_length > 0)	/* Create downsample vector */
    {
      i++;
      step=org_length/result_length; /* floor; */
      org_length=org_length - step;
      result_length--;
      v_index[i]=v_index[i-1]+step;
    }

  if(i != num_result )	/* Should never happen */
    return (1);/* error downsampling */

  for(j=0;j<=i;j++)	/* Use downsample vector to index LoResolution vector. */
    {
      v_result[j]=freqBandTableRef[v_index[j]];
    }

  return (0);
}

/**************************************************************************/
/*!
  \brief    Initialize an instance of the noise floor level estimation module.


  \return    errorCode, noError if successful

*/
/**************************************************************************/
INT
FDKsbrEnc_InitSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE  h_sbrNoiseFloorEstimate,   /*!< Handle to SBR_NOISE_FLOOR_ESTIMATE struct */
                             INT ana_max_level,                       /*!< Maximum level of the adaptive noise. */
                             const UCHAR *freqBandTable,      /*!< Frequany band table. */
                             INT nSfb,                                /*!< Number of frequency bands. */
                             INT noiseBands,                          /*!< Number of noise bands per octave. */
                             INT noiseFloorOffset,                    /*!< Noise floor offset. */
                             INT timeSlots,                           /*!< Number of time slots in a frame. */
                             UINT useSpeechConfig             /*!< Flag: adapt tuning parameters according to speech */
                            )
{
  INT i, qexp, qtmp;
  FIXP_DBL tmp, exp;

  FDKmemclear(h_sbrNoiseFloorEstimate,sizeof(SBR_NOISE_FLOOR_ESTIMATE));

  h_sbrNoiseFloorEstimate->smoothFilter = smoothFilter;
  if (useSpeechConfig) {
    h_sbrNoiseFloorEstimate->weightFac = (FIXP_DBL)MAXVAL_DBL;
    h_sbrNoiseFloorEstimate->diffThres = INVF_LOW_LEVEL;
  }
  else {
    h_sbrNoiseFloorEstimate->weightFac = FL2FXCONST_DBL(0.25f);
    h_sbrNoiseFloorEstimate->diffThres = INVF_MID_LEVEL;
  }

  h_sbrNoiseFloorEstimate->timeSlots     = timeSlots;
  h_sbrNoiseFloorEstimate->noiseBands    = noiseBands;

  /* h_sbrNoiseFloorEstimate->ana_max_level is scaled by 0.25  */
  switch(ana_max_level)
  {
  case 6:
      h_sbrNoiseFloorEstimate->ana_max_level = (FIXP_DBL)MAXVAL_DBL;
      break;
  case 3:
      h_sbrNoiseFloorEstimate->ana_max_level = FL2FXCONST_DBL(0.5);
      break;
  case -3:
      h_sbrNoiseFloorEstimate->ana_max_level = FL2FXCONST_DBL(0.125);
      break;
  default:
      /* Should not enter here */
      h_sbrNoiseFloorEstimate->ana_max_level = (FIXP_DBL)MAXVAL_DBL;
      break;
  }

  /*
    calculate number of noise bands and allocate
  */
  if(FDKsbrEnc_resetSbrNoiseFloorEstimate(h_sbrNoiseFloorEstimate,freqBandTable,nSfb))
    return(1);

  if(noiseFloorOffset == 0) {
    tmp = ((FIXP_DBL)MAXVAL_DBL)>>NOISE_FLOOR_OFFSET_SCALING;
  }
  else {
    FDK_ASSERT(noiseFloorOffset<=8); /* because of NOISE_FLOOR_OFFSET_SCALING */

      /* Assumes the noise floor offset in tuning table are in q31    */
      /* Currently the table contains only 0 for noise floor offset   */
      /* Change the qformat here when non-zero values would be filled */
    exp = fDivNorm((FIXP_DBL)noiseFloorOffset, 3, &qexp);
    tmp = fPow(2, DFRACT_BITS-1, exp, qexp, &qtmp);
    tmp = scaleValue(tmp, qtmp-NOISE_FLOOR_OFFSET_SCALING);
  }

  for(i=0;i<h_sbrNoiseFloorEstimate->noNoiseBands;i++) {
    h_sbrNoiseFloorEstimate->noiseFloorOffset[i] = tmp;
  }

  return (0);
}

/**************************************************************************/
/*!
  \brief     Resets the current instance of the noise floor estiamtion
          module.


  \return    errorCode, noError if successful

*/
/**************************************************************************/
INT
FDKsbrEnc_resetSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate, /*!< Handle to SBR_NOISE_FLOOR_ESTIMATE struct */
                            const UCHAR *freqBandTable,             /*!< Frequany band table. */
                            INT nSfb)                             /*!< Number of bands in the frequency band table. */
{
    INT k2,kx;

    /*
    * Calculate number of noise bands
    ***********************************/
    k2=freqBandTable[nSfb];
    kx=freqBandTable[0];
    if(h_sbrNoiseFloorEstimate->noiseBands == 0){
        h_sbrNoiseFloorEstimate->noNoiseBands = 1;
    }
    else{
    /*
    * Calculate number of noise bands 1,2 or 3 bands/octave
        ********************************************************/
        FIXP_DBL tmp, ratio, lg2;
        INT ratio_e, qlg2;

        ratio = fDivNorm(k2, kx, &ratio_e);
        lg2 = fLog2(ratio, ratio_e, &qlg2);
        tmp = fMult((FIXP_DBL)(h_sbrNoiseFloorEstimate->noiseBands<<24), lg2);
        tmp = scaleValue(tmp, qlg2-23);

        h_sbrNoiseFloorEstimate->noNoiseBands = (INT)((tmp + (FIXP_DBL)1) >> 1);

        if (h_sbrNoiseFloorEstimate->noNoiseBands > MAX_NUM_NOISE_COEFFS)
          h_sbrNoiseFloorEstimate->noNoiseBands = MAX_NUM_NOISE_COEFFS;

        if( h_sbrNoiseFloorEstimate->noNoiseBands==0)
            h_sbrNoiseFloorEstimate->noNoiseBands=1;
    }


    return(downSampleLoRes(h_sbrNoiseFloorEstimate->freqBandTableQmf,
        h_sbrNoiseFloorEstimate->noNoiseBands,
        freqBandTable,nSfb));
}

/**************************************************************************/
/*!
  \brief     Deletes the current instancce of the noise floor level
  estimation module.


  \return    none

*/
/**************************************************************************/
void
FDKsbrEnc_deleteSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate)  /*!< Handle to SBR_NOISE_FLOOR_ESTIMATE struct */
{

  if (h_sbrNoiseFloorEstimate) {
    /*
      nothing to do
    */
  }
}
