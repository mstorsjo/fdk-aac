/*************************  Fast MPEG AAC Audio Encoder  **********************

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
   Initial author:       M. Schug / A. Groeschel
   contents/description: bandwidth expert

******************************************************************************/

#include "channel_map.h"
#include "bandwidth.h"
#include "aacEnc_ram.h"

typedef struct{
  INT chanBitRate;
  INT bandWidthMono;
  INT bandWidth2AndMoreChan;

} BANDWIDTH_TAB;

static const BANDWIDTH_TAB bandWidthTable[] = {
  {0,      3700,  5000},
  {12000,  5000,  6400},
  {20000,  6900,  9640},
  {28000,  9600, 13050},
  {40000, 12060, 14260},
  {56000, 13950, 15500},
  {72000, 14200, 16120},
  {96000, 17000, 17000},
  {576001,17000, 17000}
};


static const BANDWIDTH_TAB bandWidthTable_LD_22050[] = {
  { 8000,  2000,  2400},
  {12000,  2500,  2700},
  {16000,  3300,  3100},
  {24000,  6250,  7200},
  {32000,  9200,  10500},
  {40000,  16000, 16000},
  {48000,  16000, 16000},
  {360001, 16000, 16000}
};

static const BANDWIDTH_TAB bandWidthTable_LD_24000[] = {
  { 8000,  2000,  2000},
  {12000,  2000,  2300},
  {16000,  2200,  2500},
  {24000,  5650,  6400},
  {32000,  11600, 12000},
  {40000,  12000, 16000},
  {48000,  16000, 16000},
  {64000,  16000, 16000},
  {360001, 16000, 16000}
};

static const BANDWIDTH_TAB bandWidthTable_LD_32000[] = {
  { 8000,  2000,  2000},
  {12000,  2000,  2000},
  {24000,  4250,  5200},
  {32000,  8400,  9000},
  {40000,  9400,  11300},
  {48000,  11900, 13700},
  {64000,  14800, 16000},
  {76000,  16000, 16000},
  {360001, 16000, 16000}
};

static const BANDWIDTH_TAB bandWidthTable_LD_44100[] = {
  { 8000,  2000,  2000},
  {24000,  2000,  2000},
  {32000,  4400,  5700},
  {40000,  7400,  8800},
  {48000,  9000,  10700},
  {56000,  11000, 12900},
  {64000,  14400, 15500},
  {80000,  16000, 16200},
  {96000,  16500, 16000},
  {128000, 16000, 16000},
  {360001, 16000, 16000}
};

static const BANDWIDTH_TAB bandWidthTable_LD_48000[] = {
  { 8000,  2000,  2000},
  {24000,  2000,  2000},
  {32000,  4400,  5700},
  {40000,  7400,  8800},
  {48000,  9000,  10700},
  {56000,  11000, 12800},
  {64000,  14300, 15400},
  {80000,  16000, 16200},
  {96000,  16500, 16000},
  {128000, 16000, 16000},
  {360001, 16000, 16000}
};

typedef struct{
  AACENC_BITRATE_MODE bitrateMode;
  int bandWidthMono;
  int bandWidth2AndMoreChan;
} BANDWIDTH_TAB_VBR;

static const BANDWIDTH_TAB_VBR bandWidthTableVBR[]= {
  {AACENC_BR_MODE_CBR,        0,     0},
  {AACENC_BR_MODE_VBR_1,  13050, 13050},
  {AACENC_BR_MODE_VBR_2,  13050, 13050},
  {AACENC_BR_MODE_VBR_3,  14260, 14260},
  {AACENC_BR_MODE_VBR_4,  15500, 15500},
  {AACENC_BR_MODE_VBR_5,  48000, 48000},
  {AACENC_BR_MODE_SFR,        0,     0},
  {AACENC_BR_MODE_FF,         0,     0}

};

static INT GetBandwidthEntry(
            const INT frameLength,
            const INT sampleRate,
            const INT chanBitRate,
            const INT entryNo)
{
  INT bandwidth = -1;
  const BANDWIDTH_TAB *pBwTab = NULL;
  INT bwTabSize = 0;

  switch (frameLength) {
    case 960:
    case 1024:
      pBwTab = bandWidthTable;
      bwTabSize = sizeof(bandWidthTable)/sizeof(BANDWIDTH_TAB);
      break;
    case 480:
    case 512:
      switch (sampleRate) {
        case 8000:
        case 11025:
        case 12000:
        case 16000:
        case 22050:
          pBwTab = bandWidthTable_LD_22050;
          bwTabSize = sizeof(bandWidthTable_LD_22050)/sizeof(BANDWIDTH_TAB);
          break;
        case 24000:
          pBwTab = bandWidthTable_LD_24000;
          bwTabSize = sizeof(bandWidthTable_LD_24000)/sizeof(BANDWIDTH_TAB);
          break;
        case 32000:
          pBwTab = bandWidthTable_LD_32000;
          bwTabSize = sizeof(bandWidthTable_LD_32000)/sizeof(BANDWIDTH_TAB);
          break;
        case (44100):
          pBwTab = bandWidthTable_LD_44100;
          bwTabSize = sizeof(bandWidthTable_LD_44100)/sizeof(BANDWIDTH_TAB);
          break;
        case 48000:
        case 64000:
        case 88200:
        case 96000:
          pBwTab = bandWidthTable_LD_48000;
          bwTabSize = sizeof(bandWidthTable_LD_48000)/sizeof(BANDWIDTH_TAB);
          break;
      }
      break;
    default:
      pBwTab = NULL;
      bwTabSize = 0;
  }

  if (pBwTab!=NULL) {
    int i;
    for (i=0; i<bwTabSize-1; i++) {
      if (chanBitRate >= pBwTab[i].chanBitRate &&
          chanBitRate < pBwTab[i+1].chanBitRate)
      {
        switch (frameLength) {
          case 960:
          case 1024:
            bandwidth = (entryNo==0)
              ? pBwTab[i].bandWidthMono
              : pBwTab[i].bandWidth2AndMoreChan;
            break;
          case 480:
          case 512:
            {
              INT q_res = 0;
              INT startBw = (entryNo==0) ?  pBwTab[i  ].bandWidthMono : pBwTab[i  ].bandWidth2AndMoreChan;
              INT endBw =   (entryNo==0) ?  pBwTab[i+1].bandWidthMono : pBwTab[i+1].bandWidth2AndMoreChan;
              INT startBr = pBwTab[i].chanBitRate;
              INT endBr = pBwTab[i+1].chanBitRate;

              FIXP_DBL bwFac_fix = fDivNorm(chanBitRate-startBr, endBr-startBr, &q_res);
              bandwidth = (INT)scaleValue(fMult(bwFac_fix, (FIXP_DBL)(endBw-startBw)),q_res) + startBw;
            }
            break;
          default:
            bandwidth = -1;
        }
        break;
      } /* within bitrate range */
    }
  } /* pBwTab!=NULL */

  return bandwidth;
}


AAC_ENCODER_ERROR FDKaacEnc_DetermineBandWidth(INT* bandWidth,
                                               INT proposedBandWidth,
                                               INT bitrate,
                                               AACENC_BITRATE_MODE bitrateMode,
                                               INT sampleRate,
                                               INT frameLength,
                                               CHANNEL_MAPPING* cm,
                                               CHANNEL_MODE encoderMode)
{
  AAC_ENCODER_ERROR ErrorStatus = AAC_ENC_OK;
  //FIXP_DBL invBandWidthGain=FL2FXCONST_DBL(1.f);
  INT chanBitRate = bitrate/cm->nChannels;

  /* vbr */
  switch(bitrateMode){
  case AACENC_BR_MODE_VBR_1:
  case AACENC_BR_MODE_VBR_2:
  case AACENC_BR_MODE_VBR_3:
  case AACENC_BR_MODE_VBR_4:
  case AACENC_BR_MODE_VBR_5:
    if (proposedBandWidth != 0){
      /* use given bw */
      *bandWidth = proposedBandWidth;
    } else {
      /* take bw from table */
      switch(encoderMode){
      case MODE_1:
        *bandWidth = bandWidthTableVBR[bitrateMode].bandWidthMono;
        break;
      case MODE_2:
      case MODE_1_2:
      case MODE_1_2_1:
      case MODE_1_2_2:
      case MODE_1_2_2_1:
      case MODE_1_2_2_2_1:
        *bandWidth = bandWidthTableVBR[bitrateMode].bandWidth2AndMoreChan;
        break;
      default:
        return AAC_ENC_UNSUPPORTED_CHANNELCONFIG;
      }
    }
    break;
  case AACENC_BR_MODE_CBR:
  case AACENC_BR_MODE_SFR:
  case AACENC_BR_MODE_FF:

    /* bandwidth limiting */
    if (proposedBandWidth != 0) {
      *bandWidth = FDKmin(proposedBandWidth, FDKmin(20000, sampleRate>>1));
    }
    else { /* search reasonable bandwidth */

      int entryNo = 0;

      switch(encoderMode){
      case MODE_1:        /* mono      */
        entryNo = 0;      /* use mono bandwith settings */
        break;

      case MODE_2:        /* stereo    */
      case MODE_1_2:      /* sce + cpe */
      case MODE_1_2_1:    /* sce + cpe + sce */
      case MODE_1_2_2:    /* sce + cpe + cpe */
      case MODE_1_2_2_1:  /* (5.1) sce + cpe + cpe + lfe */
      case MODE_1_2_2_2_1: /* (7.1) sce + cpe + cpe + cpe + lfe */
        entryNo = 1;      /* use stereo bandwith settings */
        break;

      default:
        return AAC_ENC_UNSUPPORTED_CHANNELCONFIG;
      }

      *bandWidth = GetBandwidthEntry(
            frameLength,
            sampleRate,
            chanBitRate,
            entryNo);

      if (*bandWidth==-1) {
        ErrorStatus = AAC_ENC_INVALID_CHANNEL_BITRATE;
      }
    }
    break;
  default:
    *bandWidth = 0;
    return AAC_ENC_UNSUPPORTED_BITRATE_MODE;
  }

  *bandWidth = FDKmin(*bandWidth, sampleRate/2);

  return ErrorStatus;;
}
