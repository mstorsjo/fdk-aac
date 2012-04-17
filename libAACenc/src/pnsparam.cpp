/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (2001)
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
   Initial author:       M.Lohwasser
   contents/description: PNS parameters depending on bitrate and bandwidth

******************************************************************************/

#include "pnsparam.h"
#include "psy_configuration.h"

typedef struct {
  SHORT     startFreq;
  /* Parameters for detection */
  FIXP_SGL  refPower;
  FIXP_SGL  refTonality;
  SHORT     tnsGainThreshold;     /* scaled by TNS_PREDGAIN_SCALE (=1000) */
  SHORT     tnsPNSGainThreshold;  /* scaled by TNS_PREDGAIN_SCALE (=1000) */
  FIXP_SGL  gapFillThr;
  SHORT     minSfbWidth;
  USHORT    detectionAlgorithmFlags;
} PNS_INFO_TAB;


typedef struct {
  ULONG brFrom;
  ULONG brTo;
  UCHAR S22050;
  UCHAR S24000;
  UCHAR S32000;
  UCHAR S44100;
  UCHAR S48000;
} AUTO_PNS_TAB;

static const AUTO_PNS_TAB levelTable_mono[]= {
  {0,      11999, 1, 1, 1, 1, 1,},
  {12000,  19999, 1, 1, 1, 1, 1,},
  {20000,  28999, 2, 1, 1, 1, 1,},
  {29000,  40999, 4, 4, 4, 2, 2,},
  {41000,  55999, 9, 9, 7, 7, 7,},
  {56000,  79999, 0, 0, 0, 9, 9,},
  {80000,  99999, 0, 0, 0, 0, 0,},
  {100000,999999, 0, 0, 0, 0, 0,},
};

static const AUTO_PNS_TAB levelTable_stereo[]= {
  {0,      11999, 1, 1, 1, 1, 1,},
  {12000,  19999, 3, 1, 1, 1, 1,},
  {20000,  28999, 3, 3, 3, 2, 2,},
  {29000,  40999, 7, 6, 6, 5, 5,},
  {41000,  55999, 9, 9, 7, 7, 7,},
  {56000,  79999, 0, 0, 0, 0, 0,},
  {80000,  99999, 0, 0, 0, 0, 0,},
  {100000,999999, 0, 0, 0, 0, 0,},
};


static const PNS_INFO_TAB pnsInfoTab[] = {
/*0   pns off */
/*1*/ { 4000, FL2FXCONST_SGL(0.04), FL2FXCONST_SGL(0.06), 1150, 1200, FL2FXCONST_SGL(0.02), 8,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS /*| JUST_LONG_WINDOW*/ },
/*2*/ { 4000, FL2FXCONST_SGL(0.04), FL2FXCONST_SGL(0.07), 1130, 1300, FL2FXCONST_SGL(0.05), 8,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS /*| JUST_LONG_WINDOW*/ },
/*3*/ { 4100, FL2FXCONST_SGL(0.04), FL2FXCONST_SGL(0.07), 1100, 1400, FL2FXCONST_SGL(0.10), 8,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS /*| JUST_LONG_WINDOW*/ },
/*4*/ { 4100, FL2FXCONST_SGL(0.03), FL2FXCONST_SGL(0.10), 1100, 1400, FL2FXCONST_SGL(0.15), 8,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS /*| JUST_LONG_WINDOW*/ },
/*5*/ { 4300, FL2FXCONST_SGL(0.03), FL2FXCONST_SGL(0.10), 1100, 1400, FL2FXCONST_SGL(0.15), 8,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS | JUST_LONG_WINDOW },
/*6*/ { 5000, FL2FXCONST_SGL(0.03), FL2FXCONST_SGL(0.10), 1100, 1400, FL2FXCONST_SGL(0.25), 8,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS | JUST_LONG_WINDOW },
/*7*/ { 5500, FL2FXCONST_SGL(0.03), FL2FXCONST_SGL(0.12), 1100, 1400, FL2FXCONST_SGL(0.35), 8,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS | JUST_LONG_WINDOW },
/*8*/ { 6000, FL2FXCONST_SGL(0.03), FL2FXCONST_SGL(0.12), 1080, 1400, FL2FXCONST_SGL(0.40), 8,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS | JUST_LONG_WINDOW },
/*9*/ { 6000, FL2FXCONST_SGL(0.03), FL2FXCONST_SGL(0.14), 1070, 1400, FL2FXCONST_SGL(0.45), 8,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS | JUST_LONG_WINDOW },
};

static const AUTO_PNS_TAB levelTable_lowComplexity[]= {
  //{0,      23999, 1, 1, 1, 1, 1,},
  //{24000,  31999, 2, 2, 2, 2, 2,},
  {0,      27999, 0, 0, 0, 0, 0,},
  {28000,  31999, 2, 2, 2, 2, 2,},
  {32000,  47999, 3, 3, 3, 3, 3,},
  {48000,  48000, 4, 4, 4, 4, 4,},
  {48001, 999999, 0, 0, 0, 0, 0,},
};

/* conversion of old LC tuning tables to new (LD enc) structure (only entries which are actually used were converted) */
static const PNS_INFO_TAB pnsInfoTab_lowComplexity[] = {
/*0   pns off */
      /* DEFAULT parameter set */
/*1*/ { 4100, FL2FXCONST_SGL(0.03), FL2FXCONST_SGL(0.16), 1100, 1400, FL2FXCONST_SGL(0.5), 16,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS | JUST_LONG_WINDOW },
/*2*/ { 4100, FL2FXCONST_SGL(0.05), FL2FXCONST_SGL(0.10), 1410, 1400, FL2FXCONST_SGL(0.5), 16,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS | JUST_LONG_WINDOW },
/*3*/ { 4100, FL2FXCONST_SGL(0.05), FL2FXCONST_SGL(0.10), 1100, 1400, FL2FXCONST_SGL(0.5), 16,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS | JUST_LONG_WINDOW },
      /* LOWSUBST -> PNS is used less often than with DEFAULT parameter set (for br: 48000 - 79999) */
/*4*/ { 4100, FL2FXCONST_SGL(0.20), FL2FXCONST_SGL(0.10), 1410, 1400, FL2FXCONST_SGL(0.5), 16,
        USE_POWER_DISTRIBUTION | USE_PSYCH_TONALITY | USE_TNS_GAIN_THR | USE_TNS_PNS | JUST_LONG_WINDOW },
};

/****************************************************************************
  function to look up used pns level
****************************************************************************/
int FDKaacEnc_lookUpPnsUse (int bitRate, int sampleRate, int numChan, const int isLC) {

  int hUsePns=0, size, i;
  const AUTO_PNS_TAB *levelTable;

  if (isLC) {
    levelTable = &levelTable_lowComplexity[0];
    size = sizeof(levelTable_lowComplexity);
  } else
  { /* (E)LD */
    levelTable = (numChan > 1) ? &levelTable_stereo[0] : &levelTable_mono[0];
    size = (numChan > 1) ? sizeof(levelTable_stereo) : sizeof(levelTable_mono);
  }

  for(i = 0; i < (int) (size/sizeof(AUTO_PNS_TAB)); i++) {
    if(((ULONG)bitRate >= levelTable[i].brFrom) &&
       ((ULONG)bitRate <= levelTable[i].brTo) )
      break;
  }

  /* sanity check */
  if ((int)(sizeof(pnsInfoTab)/sizeof(PNS_INFO_TAB)) < i ) {
    return (PNS_TABLE_ERROR);
  }

  switch (sampleRate) {
  case 22050: hUsePns = levelTable[i].S22050; break;
  case 24000: hUsePns = levelTable[i].S24000; break;
  case 32000: hUsePns = levelTable[i].S32000; break;
  case 44100: hUsePns = levelTable[i].S44100; break;
  case 48000: hUsePns = levelTable[i].S48000; break;
  default:
    if (isLC) {
      hUsePns = levelTable[i].S48000;
    }
    break;
  }

  return (hUsePns);
}


/*****************************************************************************

    functionname: FDKaacEnc_GetPnsParam
    description:  Gets PNS parameters depending on bitrate and bandwidth
    returns:      error status
    input:        Noiseparams struct, bitrate, sampling rate,
                  number of sfb's, pointer to sfb offset
    output:       PNS parameters

*****************************************************************************/
AAC_ENCODER_ERROR FDKaacEnc_GetPnsParam(NOISEPARAMS *np,
                                        INT         bitRate,
                                        INT         sampleRate,
                                        INT         sfbCnt,
                                        const INT   *sfbOffset,
                                        INT         *usePns,
                                        INT         numChan,
                                        const int   isLC)

{
    int i, hUsePns;
    const PNS_INFO_TAB *pnsInfo;

    if (isLC) {
      np->detectionAlgorithmFlags = IS_LOW_COMLEXITY;
      pnsInfo = pnsInfoTab_lowComplexity;
    }
    else
    {
      np->detectionAlgorithmFlags = 0;
      pnsInfo = pnsInfoTab;
    }

    if (*usePns<=0)
        return AAC_ENC_OK;

    /* new pns params */
    hUsePns = FDKaacEnc_lookUpPnsUse (bitRate, sampleRate, numChan, isLC);
    if (hUsePns == 0) {
      *usePns = 0;
        return AAC_ENC_OK;
        }
    if (hUsePns == PNS_TABLE_ERROR)
      return AAC_ENC_PNS_TABLE_ERROR;

    /* select correct row of tuning table */
    pnsInfo += hUsePns-1;

    np->startSfb = FDKaacEnc_FreqToBandWithRounding( pnsInfo->startFreq,
        sampleRate,
        sfbCnt,
        sfbOffset );

    np->detectionAlgorithmFlags |= pnsInfo->detectionAlgorithmFlags;

    np->refPower             = FX_SGL2FX_DBL(pnsInfo->refPower);
    np->refTonality          = FX_SGL2FX_DBL(pnsInfo->refTonality);
    np->tnsGainThreshold     = pnsInfo->tnsGainThreshold;
    np->tnsPNSGainThreshold  = pnsInfo->tnsPNSGainThreshold;
    np->minSfbWidth          = pnsInfo->minSfbWidth;

    np->gapFillThr           = (FIXP_SGL)pnsInfo->gapFillThr;

    /* assuming a constant dB/Hz slope in the signal's PSD curve,
    the detection threshold needs to be corrected for the width of the band */
    for ( i = 0; i < (sfbCnt-1); i++)
    {
        INT qtmp, sfbWidth;
        FIXP_DBL tmp;

        sfbWidth = sfbOffset[i+1]-sfbOffset[i];

        tmp = fPow(np->refPower, 0, sfbWidth, DFRACT_BITS-1-5, &qtmp);
        np->powDistPSDcurve[i] = (FIXP_SGL)((LONG)(scaleValue(tmp, qtmp) >> 16));
    }
    np->powDistPSDcurve[sfbCnt] = np->powDistPSDcurve[sfbCnt-1];

  return AAC_ENC_OK;
}
