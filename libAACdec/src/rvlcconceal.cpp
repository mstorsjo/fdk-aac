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
  \brief  rvlc concealment
  \author Josef Hoepfl
*/

#include "rvlcconceal.h"


#include "block.h"
#include "rvlc.h"

/*---------------------------------------------------------------------------------------------
  function:      calcRefValFwd

  description:   The function determines the scalefactor which is closed to the scalefactorband
                 conceal_min. The same is done for intensity data and noise energies.
-----------------------------------------------------------------------------------------------
  output:        - reference value scf
                 - reference value internsity data
                 - reference value noise energy
-----------------------------------------------------------------------------------------------
  return:        -
-------------------------------------------------------------------------------------------- */

static
void calcRefValFwd (CErRvlcInfo *pRvlc,
                    CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                    int *refIsFwd,
                    int *refNrgFwd,
                    int *refScfFwd)
{
  int band,bnds,group,startBand;
  int idIs,idNrg,idScf;
  int conceal_min,conceal_group_min;
  int MaximumScaleFactorBands;


  if (GetWindowSequence(&pAacDecoderChannelInfo->icsInfo) == EightShortSequence)
    MaximumScaleFactorBands = 16;
  else
    MaximumScaleFactorBands = 64;

  conceal_min = pRvlc->conceal_min % MaximumScaleFactorBands;
  conceal_group_min = pRvlc->conceal_min / MaximumScaleFactorBands;

  /* calculate first reference value for approach in forward direction */
  idIs = idNrg = idScf = 1;

  /* set reference values */
  *refIsFwd = - SF_OFFSET;
  *refNrgFwd = pAacDecoderChannelInfo->pDynData->RawDataInfo.GlobalGain - SF_OFFSET - 90 - 256;
  *refScfFwd = pAacDecoderChannelInfo->pDynData->RawDataInfo.GlobalGain - SF_OFFSET;

  startBand = conceal_min-1;
  for (group=conceal_group_min; group >= 0; group--) {
    for (band=startBand; band >= 0; band--) {
      bnds = 16*group+band;
      switch (pAacDecoderChannelInfo->pDynData->aCodeBook[bnds]) {
        case ZERO_HCB:
          break;
        case INTENSITY_HCB:
        case INTENSITY_HCB2:
          if (idIs) {
            *refIsFwd = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
            idIs=0; /* reference value has been set */
          }
          break;
        case NOISE_HCB:
          if (idNrg) {
            *refNrgFwd = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
            idNrg=0; /* reference value has been set */
          }
          break ;
        default:
          if (idScf) {
            *refScfFwd = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
            idScf=0; /* reference value has been set */
          }
          break;
      }
    }
    startBand = pRvlc->maxSfbTransmitted-1;
  }

}

/*---------------------------------------------------------------------------------------------
  function:      calcRefValBwd

  description:   The function determines the scalefactor which is closed to the scalefactorband
                 conceal_max. The same is done for intensity data and noise energies.
-----------------------------------------------------------------------------------------------
  output:        - reference value scf
                 - reference value internsity data
                 - reference value noise energy
-----------------------------------------------------------------------------------------------
  return:        -
-------------------------------------------------------------------------------------------- */

static
void calcRefValBwd (CErRvlcInfo *pRvlc,
                    CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                    int *refIsBwd,
                    int *refNrgBwd,
                    int *refScfBwd)
{
  int band,bnds,group,startBand;
  int idIs,idNrg,idScf;
  int conceal_max,conceal_group_max;
  int MaximumScaleFactorBands;

  if (GetWindowSequence(&pAacDecoderChannelInfo->icsInfo) == EightShortSequence)
    MaximumScaleFactorBands = 16;
  else
    MaximumScaleFactorBands = 64;

  conceal_max = pRvlc->conceal_max % MaximumScaleFactorBands;
  conceal_group_max = pRvlc->conceal_max / MaximumScaleFactorBands;

  /* calculate first reference value for approach in backward direction */
  idIs = idNrg = idScf = 1;

  /* set reference values */
  *refIsBwd = pRvlc->dpcm_is_last_position - SF_OFFSET;
  *refNrgBwd = pRvlc->rev_global_gain + pRvlc->dpcm_noise_last_position - SF_OFFSET - 90 - 256 + pRvlc->dpcm_noise_nrg;
  *refScfBwd = pRvlc->rev_global_gain - SF_OFFSET;

  startBand=conceal_max+1;

  /* if needed, re-set reference values */
  for (group=conceal_group_max; group < pRvlc->numWindowGroups; group++) {
    for (band=startBand; band < pRvlc->maxSfbTransmitted; band++) {
      bnds = 16*group+band;
      switch (pAacDecoderChannelInfo->pDynData->aCodeBook[bnds]) {
        case ZERO_HCB:
          break;
        case INTENSITY_HCB:
        case INTENSITY_HCB2:
          if (idIs) {
            *refIsBwd = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
            idIs=0; /* reference value has been set */
          }
          break;
        case NOISE_HCB:
          if (idNrg) {
            *refNrgBwd = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
            idNrg=0;  /* reference value has been set */
          }
          break ;
        default:
          if (idScf) {
            *refScfBwd = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
            idScf=0; /* reference value has been set */
          }
          break;
      }
    }
    startBand=0;
  }

}


/*---------------------------------------------------------------------------------------------
  function:      BidirectionalEstimation_UseLowerScfOfCurrentFrame

  description:   This approach by means of bidirectional estimation is generally performed when
                 a single bit error has been detected, the bit error can be isolated between
                 'conceal_min' and 'conceal_max' and the 'sf_concealment' flag is not set. The
                 sets of scalefactors decoded in forward and backward direction are compared
                 with each other. The smaller scalefactor will be considered as the correct one
                 respectively. The reconstruction of the scalefactors with this approach archieve
                 good results in audio quality. The strategy must be applied to scalefactors,
                 intensity data and noise energy seperately.
-----------------------------------------------------------------------------------------------
  output:        Concealed scalefactor, noise energy and intensity data between conceal_min and
                 conceal_max
-----------------------------------------------------------------------------------------------
  return:        -
-------------------------------------------------------------------------------------------- */

void BidirectionalEstimation_UseLowerScfOfCurrentFrame (CAacDecoderChannelInfo *pAacDecoderChannelInfo)
{
  CErRvlcInfo *pRvlc = &pAacDecoderChannelInfo->pComData->overlay.aac.erRvlcInfo;
  int band,bnds,startBand,endBand,group;
  int conceal_min,conceal_max;
  int conceal_group_min,conceal_group_max;
  int MaximumScaleFactorBands;

  if (GetWindowSequence(&pAacDecoderChannelInfo->icsInfo) == EightShortSequence) {
    MaximumScaleFactorBands = 16;
  }
  else {
    MaximumScaleFactorBands = 64;
  }
  
  /* If an error was detected just in forward or backward direction, set the corresponding border for concealment to a
     appropriate scalefactor band. The border is set to first or last sfb respectively, because the error will possibly 
     not follow directly after the corrupt bit but just after decoding some more (wrong) scalefactors. */
  if (pRvlc->conceal_min == CONCEAL_MIN_INIT)
    pRvlc->conceal_min = 0;

  if (pRvlc->conceal_max == CONCEAL_MAX_INIT)
    pRvlc->conceal_max = (pRvlc->numWindowGroups-1)*16+pRvlc->maxSfbTransmitted-1;

  conceal_min = pRvlc->conceal_min % MaximumScaleFactorBands;
  conceal_group_min = pRvlc->conceal_min / MaximumScaleFactorBands;
  conceal_max = pRvlc->conceal_max % MaximumScaleFactorBands;
  conceal_group_max = pRvlc->conceal_max / MaximumScaleFactorBands;

  if (pRvlc->conceal_min == pRvlc->conceal_max) {

    int refIsFwd,refNrgFwd,refScfFwd;
    int refIsBwd,refNrgBwd,refScfBwd;

    bnds = pRvlc->conceal_min;
    calcRefValFwd(pRvlc,pAacDecoderChannelInfo,&refIsFwd,&refNrgFwd,&refScfFwd);
    calcRefValBwd(pRvlc,pAacDecoderChannelInfo,&refIsBwd,&refNrgBwd,&refScfBwd);

    switch (pAacDecoderChannelInfo->pDynData->aCodeBook[bnds]) {
      case ZERO_HCB:
        break;
      case INTENSITY_HCB:
      case INTENSITY_HCB2:
        if (refIsFwd < refIsBwd) 
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = refIsFwd;
        else
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = refIsBwd;
        break;
      case NOISE_HCB:
        if (refNrgFwd < refNrgBwd)
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = refNrgFwd;
        else
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = refNrgBwd;
        break;
      default:
        if (refScfFwd < refScfBwd)
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = refScfFwd;
        else 
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = refScfBwd;
        break;
    }
  }
  else {
    pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[pRvlc->conceal_max] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[pRvlc->conceal_max];
    pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[pRvlc->conceal_min] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[pRvlc->conceal_min];

    /* consider the smaller of the forward and backward decoded value as the correct one */  
    startBand = conceal_min;      
    if (conceal_group_min == conceal_group_max)   
      endBand = conceal_max;      
    else          
      endBand = pRvlc->maxSfbTransmitted-1;       

    for (group=conceal_group_min; group <= conceal_group_max; group++) {  
      for (band=startBand; band <= endBand; band++) {  
        bnds = 16*group+band;  
        if (pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds] < pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds])  
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
        else
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
      }  
      startBand = 0;   
      if ((group+1) == conceal_group_max)  
        endBand = conceal_max;  
    }  
  }

  /* now copy all data to the output buffer which needs not to be concealed */
  if (conceal_group_min == 0) 
    endBand = conceal_min;    
  else        
    endBand = pRvlc->maxSfbTransmitted;     
  for (group=0; group <= conceal_group_min; group++) {
    for (band=0; band < endBand; band++) {
      bnds = 16*group+band;
      pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
    }
    if ((group+1) == conceal_group_min) 
      endBand = conceal_min;    
  }

  startBand = conceal_max+1;    
  for (group=conceal_group_max; group < pRvlc->numWindowGroups; group++) {
    for (band=startBand; band < pRvlc->maxSfbTransmitted; band++) {
      bnds = 16*group+band;
      pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
    }
    startBand = 0;
  }
}

/*---------------------------------------------------------------------------------------------
  function:      BidirectionalEstimation_UseScfOfPrevFrameAsReference

  description:   This approach by means of bidirectional estimation is generally performed when
                 a single bit error has been detected, the bit error can be isolated between 
                 'conceal_min' and 'conceal_max', the 'sf_concealment' flag is set and the 
                 previous frame has the same block type as the current frame. The scalefactor 
                 decoded in forward and backward direction and the scalefactor of the previous 
                 frame are compared with each other. The smaller scalefactor will be considered 
                 as the correct one. At this the codebook of the previous and current frame must 
                 be of the same set (scf, nrg, is) in each scalefactorband. Otherwise the 
                 scalefactor of the previous frame is not considered in the minimum calculation. 
                 The reconstruction of the scalefactors with this approach archieve good results 
                 in audio quality. The strategy must be applied to scalefactors, intensity data 
                 and noise energy seperately.
-----------------------------------------------------------------------------------------------
  output:        Concealed scalefactor, noise energy and intensity data between conceal_min and 
                 conceal_max
-----------------------------------------------------------------------------------------------
  return:        -
-------------------------------------------------------------------------------------------- */

void BidirectionalEstimation_UseScfOfPrevFrameAsReference (
        CAacDecoderChannelInfo *pAacDecoderChannelInfo,
        CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo
        )
{
  CErRvlcInfo *pRvlc = &pAacDecoderChannelInfo->pComData->overlay.aac.erRvlcInfo;
  int band,bnds,startBand,endBand,group;
  int conceal_min,conceal_max;
  int conceal_group_min,conceal_group_max;
  int MaximumScaleFactorBands;
  int commonMin;

  if (GetWindowSequence(&pAacDecoderChannelInfo->icsInfo) == EightShortSequence) {
    MaximumScaleFactorBands = 16;
  }
  else {
    MaximumScaleFactorBands = 64;
  }

  /* If an error was detected just in forward or backward direction, set the corresponding border for concealment to a
     appropriate scalefactor band. The border is set to first or last sfb respectively, because the error will possibly 
     not follow directly after the corrupt bit but just after decoding some more (wrong) scalefactors. */
  if (pRvlc->conceal_min == CONCEAL_MIN_INIT)
    pRvlc->conceal_min = 0;

  if (pRvlc->conceal_max == CONCEAL_MAX_INIT)
    pRvlc->conceal_max = (pRvlc->numWindowGroups-1)*16+pRvlc->maxSfbTransmitted-1;

  conceal_min = pRvlc->conceal_min % MaximumScaleFactorBands;
  conceal_group_min = pRvlc->conceal_min / MaximumScaleFactorBands;
  conceal_max = pRvlc->conceal_max % MaximumScaleFactorBands;
  conceal_group_max = pRvlc->conceal_max / MaximumScaleFactorBands;

  pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[pRvlc->conceal_max] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[pRvlc->conceal_max];  
  pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[pRvlc->conceal_min] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[pRvlc->conceal_min];  

  /* consider the smaller of the forward and backward decoded value as the correct one */
  startBand = conceal_min;    
  if (conceal_group_min == conceal_group_max) 
    endBand = conceal_max;    
  else        
    endBand = pRvlc->maxSfbTransmitted-1;     

  for (group=conceal_group_min; group <= conceal_group_max; group++) {
    for (band=startBand; band <= endBand; band++) {
      bnds = 16*group+band;
      switch (pAacDecoderChannelInfo->pDynData->aCodeBook[bnds]) {
        case ZERO_HCB:
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = 0;
          break;

        case INTENSITY_HCB:
        case INTENSITY_HCB2:
          if ( (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]==INTENSITY_HCB) || (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]==INTENSITY_HCB2) ) {
            commonMin = FDKmin(pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds],pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds]);
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = FDKmin(commonMin, pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousScaleFactor[bnds]);
          }
          else {
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = FDKmin(pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds],pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds]);
          }
          break;

        case NOISE_HCB:
          if ( (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]==NOISE_HCB) ) {
            commonMin = FDKmin(pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds],pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds]);
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = FDKmin(commonMin, pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousScaleFactor[bnds]);
          } else {
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = FDKmin(pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds],pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds]);
          }
          break;

        default:
          if (   (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]!=ZERO_HCB)
              && (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]!=NOISE_HCB)
              && (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]!=INTENSITY_HCB) 
              && (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]!=INTENSITY_HCB2) )
          {
            commonMin = FDKmin(pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds], pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds]);
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = FDKmin(commonMin, pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousScaleFactor[bnds]);
          } else {
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = FDKmin(pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds],pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds]);
          }
          break;
      }
    }
    startBand = 0; 
    if ((group+1) == conceal_group_max)
      endBand = conceal_max;
  }

  /* now copy all data to the output buffer which needs not to be concealed */
  if (conceal_group_min == 0) 
    endBand = conceal_min;    
  else        
    endBand = pRvlc->maxSfbTransmitted;     
  for (group=0; group <= conceal_group_min; group++) {
    for (band=0; band < endBand; band++) {
      bnds = 16*group+band;
      pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
    }
    if ((group+1) == conceal_group_min) 
      endBand = conceal_min;    
  }

  startBand = conceal_max+1;    
  for (group=conceal_group_max; group < pRvlc->numWindowGroups; group++) {
    for (band=startBand; band < pRvlc->maxSfbTransmitted; band++) {
      bnds = 16*group+band;
      pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
    }
    startBand = 0;
  }
}

/*---------------------------------------------------------------------------------------------
  function:      StatisticalEstimation

  description:   This approach by means of statistical estimation is generally performed when 
                 both the start value and the end value are different and no further errors have 
                 been detected. Considering the forward and backward decoded scalefactors, the 
                 set with the lower scalefactors in sum will be considered as the correct one. 
                 The scalefactors are differentially encoded. Normally it would reach to compare 
                 one pair of the forward and backward decoded scalefactors to specify the lower 
                 set. But having detected no further errors does not necessarily mean the absence
                 of errors. Therefore all scalefactors decoded in forward and backward direction 
                 are summed up seperately. The set with the lower sum will be used. The strategy 
                 must be applied to scalefactors, intensity data and noise energy seperately.
-----------------------------------------------------------------------------------------------
  output:        Concealed scalefactor, noise energy and intensity data
-----------------------------------------------------------------------------------------------
  return:        -
-------------------------------------------------------------------------------------------- */

void StatisticalEstimation (CAacDecoderChannelInfo *pAacDecoderChannelInfo)
{
  CErRvlcInfo *pRvlc = &pAacDecoderChannelInfo->pComData->overlay.aac.erRvlcInfo;
  int band,bnds,group;
  int sumIsFwd,sumIsBwd;            /* sum of intensity data forward/backward */
  int sumNrgFwd,sumNrgBwd;          /* sum of noise energy data forward/backward */
  int sumScfFwd,sumScfBwd;          /* sum of scalefactor data forward/backward */
  int useIsFwd,useNrgFwd,useScfFwd; /* the flags signals the elements which are used for the final result */
  int MaximumScaleFactorBands;

  if (GetWindowSequence(&pAacDecoderChannelInfo->icsInfo) == EightShortSequence)
    MaximumScaleFactorBands = 16;
  else
    MaximumScaleFactorBands = 64;

  sumIsFwd = sumIsBwd = sumNrgFwd = sumNrgBwd = sumScfFwd = sumScfBwd = 0;
  useIsFwd = useNrgFwd = useScfFwd = 0;

  /* calculate sum of each group (scf,nrg,is) of forward and backward direction */
  for (group=0; group<pRvlc->numWindowGroups; group++) {
    for (band=0; band < pRvlc->maxSfbTransmitted; band++) {
      bnds = 16*group+band;
      switch (pAacDecoderChannelInfo->pDynData->aCodeBook[bnds]) {
        case ZERO_HCB:
          break;

        case INTENSITY_HCB:
        case INTENSITY_HCB2:
          sumIsFwd += pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
          sumIsBwd += pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
          break;

        case NOISE_HCB:
          sumNrgFwd += pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
          sumNrgBwd += pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
          break ;

        default:
          sumScfFwd += pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
          sumScfBwd += pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
          break;
      }
    }
  }

  /* find for each group (scf,nrg,is) the correct direction */
  if ( sumIsFwd < sumIsBwd )
    useIsFwd = 1;

  if ( sumNrgFwd < sumNrgBwd )
    useNrgFwd = 1;

  if ( sumScfFwd < sumScfBwd )
    useScfFwd = 1;

  /* conceal each group (scf,nrg,is) */
  for (group=0; group<pRvlc->numWindowGroups; group++) {
    for (band=0; band < pRvlc->maxSfbTransmitted; band++) {
      bnds = 16*group+band;
      switch (pAacDecoderChannelInfo->pDynData->aCodeBook[bnds]) {
        case ZERO_HCB:
          break;

        case INTENSITY_HCB:
        case INTENSITY_HCB2:
          if (useIsFwd)
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
          else
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
          break;

        case NOISE_HCB:
          if (useNrgFwd)
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
          else
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
          break ;

        default:
          if (useScfFwd)
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds];
          else
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds];
          break;
      }
    }
  }
}


/*---------------------------------------------------------------------------------------------
  description:   Approach by means of predictive interpolation
                 This approach by means of predictive estimation is generally performed when 
                 the error cannot be isolated between 'conceal_min' and 'conceal_max', the 
                 'sf_concealment' flag is set and the previous frame has the same block type 
                 as the current frame. Check for each scalefactorband if the same type of data 
                 (scalefactor, internsity data, noise energies) is transmitted. If so use the 
                 scalefactor (intensity data, noise energy) in the current frame. Otherwise set 
                 the scalefactor (intensity data, noise energy) for this scalefactorband to zero.
-----------------------------------------------------------------------------------------------
  output:        Concealed scalefactor, noise energy and intensity data
-----------------------------------------------------------------------------------------------
  return:        -
-------------------------------------------------------------------------------------------- */

void PredictiveInterpolation (
        CAacDecoderChannelInfo *pAacDecoderChannelInfo,
        CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo
        )
{
  CErRvlcInfo *pRvlc = &pAacDecoderChannelInfo->pComData->overlay.aac.erRvlcInfo;
  int band,bnds,group;
  int MaximumScaleFactorBands;
  int commonMin;

  if (GetWindowSequence(&pAacDecoderChannelInfo->icsInfo) == EightShortSequence)
    MaximumScaleFactorBands = 16;
  else
    MaximumScaleFactorBands = 64;

  for (group=0; group<pRvlc->numWindowGroups; group++) {
    for (band=0; band < pRvlc->maxSfbTransmitted; band++) {
      bnds = 16*group+band;
      switch (pAacDecoderChannelInfo->pDynData->aCodeBook[bnds]) {
        case ZERO_HCB:
          pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = 0;
          break;

        case INTENSITY_HCB:
        case INTENSITY_HCB2:
          if ( (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]==INTENSITY_HCB) || (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]==INTENSITY_HCB2) ) {
            commonMin = FDKmin(pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds],pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds]);
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = FDKmin(commonMin, pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousScaleFactor[bnds]);
          }
          else {
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = -110;
          }
          break;

        case NOISE_HCB:
          if ( (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]==NOISE_HCB) ) {
            commonMin = FDKmin(pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds],pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds]);
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = FDKmin(commonMin, pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousScaleFactor[bnds]);
          }
          else {
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = -110;
          }
          break;

        default:
          if (   (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]!=ZERO_HCB)
              && (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]!=NOISE_HCB) 
              && (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]!=INTENSITY_HCB) 
              && (pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousCodebook[bnds]!=INTENSITY_HCB2) ) {
            commonMin = FDKmin(pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfFwd[bnds],pAacDecoderChannelInfo->pComData->overlay.aac.aRvlcScfBwd[bnds]);
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = FDKmin(commonMin, pAacDecoderStaticChannelInfo->concealmentInfo.aRvlcPreviousScaleFactor[bnds]);
          }
          else {
            pAacDecoderChannelInfo->pDynData->aScaleFactor[bnds] = 0;
          }
          break;
      }
    }
  }
}

