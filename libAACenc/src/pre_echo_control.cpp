/******************************** MPEG Audio Encoder **************************

                       (C) copyright Fraunhofer IIS (1999)
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
   Initial author:       M.Werner
   contents/description: Pre echo control

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "pre_echo_control.h"
#include "psy_configuration.h"

void FDKaacEnc_InitPreEchoControl(FIXP_DBL    *RESTRICT pbThresholdNm1,
                                  INT         *calcPreEcho,
                                  INT          numPb,
                                  FIXP_DBL    *RESTRICT sfbPcmQuantThreshold,
                                  INT         *mdctScalenm1)
{
  *mdctScalenm1 = PCM_QUANT_THR_SCALE>>1;

  FDKmemcpy(pbThresholdNm1, sfbPcmQuantThreshold, numPb*sizeof(FIXP_DBL));

  *calcPreEcho = 1;
}


void FDKaacEnc_PreEchoControl(FIXP_DBL   *RESTRICT pbThresholdNm1,
                              INT         calcPreEcho,
                              INT         numPb,
                              INT         maxAllowedIncreaseFactor,
                              FIXP_SGL    minRemainingThresholdFactor,
                              FIXP_DBL   *RESTRICT pbThreshold,
                              INT         mdctScale,
                              INT        *mdctScalenm1)
{
    int i;
    FIXP_DBL tmpThreshold1, tmpThreshold2;
    int scaling;

	/* If lastWindowSequence in previous frame was start- or stop-window,
	   skip preechocontrol calculation */
    if (calcPreEcho==0) {
      /* copy thresholds to internal memory */
      FDKmemcpy(pbThresholdNm1, pbThreshold, numPb*sizeof(FIXP_DBL));
      *mdctScalenm1 = mdctScale;
      return;
    }

    if ( mdctScale > *mdctScalenm1 ) {
        /* if current thresholds are downscaled more than the ones from the last block */
        scaling = 2*(mdctScale-*mdctScalenm1);
        for(i = 0; i < numPb; i++) {

            /* multiplication with return data type fract ist equivalent to int multiplication */
            FDK_ASSERT(scaling>=0);
            tmpThreshold1 = maxAllowedIncreaseFactor * (pbThresholdNm1[i]>>scaling);
            tmpThreshold2 = fMult(minRemainingThresholdFactor, pbThreshold[i]);

			FIXP_DBL tmp = pbThreshold[i];

            /* copy thresholds to internal memory */
            pbThresholdNm1[i] = tmp;

            tmp = fixMin(tmp, tmpThreshold1);
            pbThreshold[i] = fixMax(tmp, tmpThreshold2);
        }
    }
    else {
        /* if thresholds of last block are more downscaled than the current ones */
        scaling = 2*(*mdctScalenm1-mdctScale);
        for(i = 0; i < numPb; i++) {

            /* multiplication with return data type fract ist equivalent to int multiplication */
            tmpThreshold1 = (maxAllowedIncreaseFactor>>1) * pbThresholdNm1[i];
            tmpThreshold2 = fMult(minRemainingThresholdFactor, pbThreshold[i]);

            /* copy thresholds to internal memory */
            pbThresholdNm1[i] = pbThreshold[i];

            FDK_ASSERT(scaling>=0);
            if((pbThreshold[i]>>(scaling+1)) > tmpThreshold1) {
                pbThreshold[i] = tmpThreshold1<<(scaling+1);
            }
            pbThreshold[i] = fixMax(pbThreshold[i], tmpThreshold2);
        }
    }

    *mdctScalenm1 = mdctScale;
}
