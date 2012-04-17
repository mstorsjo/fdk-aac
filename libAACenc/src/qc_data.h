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
   Initial author:       M. Werner
   contents/description: Quantizing & coding data

******************************************************************************/
#ifndef _QC_DATA_H
#define _QC_DATA_H


#include "psy_const.h"
#include "dyn_bits.h"
#include "adj_thr_data.h"
#include "line_pe.h"
#include "FDK_audio.h"
#include "interface.h"


typedef enum {
  QCDATA_BR_MODE_INVALID = -1,
  QCDATA_BR_MODE_CBR     =  0,
  QCDATA_BR_MODE_VBR_1   =  1, /* 32 kbps/channel */
  QCDATA_BR_MODE_VBR_2   =  2, /* 40 kbps/channel */
  QCDATA_BR_MODE_VBR_3   =  3, /* 48 kbps/channel */
  QCDATA_BR_MODE_VBR_4   =  4, /* 64 kbps/channel */
  QCDATA_BR_MODE_VBR_5   =  5, /* 96 kbps/channel */
  QCDATA_BR_MODE_FF      =  6,  /* Fixed frame mode. */
  QCDATA_BR_MODE_SFR     =  7  /* Superframe mode. */


} QCDATA_BR_MODE;

typedef struct {
  MP4_ELEMENT_ID elType;
  INT instanceTag;
  INT nChannelsInEl;
  INT ChannelIndex[2];
  FIXP_DBL relativeBits;
} ELEMENT_INFO;

typedef struct {
  CHANNEL_MODE encMode;
  INT nChannels;
  INT nChannelsEff;
  INT nElements;
  ELEMENT_INFO elInfo[(6)];
//  INT elDSE;                   /* DSE element signalling */
} CHANNEL_MAPPING;

typedef struct {
  INT paddingRest;
} PADDING;


/* Quantizing & coding stage */

struct QC_INIT{
  CHANNEL_MAPPING* channelMapping;
  INT sceCpe;      /* not used yet                         */
  INT maxBits;     /* maximum number of bits in reservoir  */
  INT averageBits; /* average number of bits we should use */
  INT bitRes;
  INT staticBits;  /* Bits per frame consumed by transport layers. */
  QCDATA_BR_MODE bitrateMode;
  INT meanPe;
  INT chBitrate;
  INT invQuant;
  INT maxIterations; /* Maximum number of allowed iterations before FDKaacEnc_crashRecovery() is applied. */
  FIXP_DBL maxBitFac;
  INT bitrate;
  INT nSubFrames; /* helper variable */
  INT minBits;    /* minimal number of bits in one frame*/

  PADDING padding;
};

typedef struct
{
  FIXP_DBL      mdctSpectrum[(1024)];

  SHORT         quantSpec[(1024)];

  UINT          maxValueInSfb[MAX_GROUPED_SFB];
  INT           scf[MAX_GROUPED_SFB];
  INT           globalGain;
  SECTION_DATA  sectionData;

  FIXP_DBL      sfbFormFactorLdData[MAX_GROUPED_SFB];

  FIXP_DBL      sfbThresholdLdData[MAX_GROUPED_SFB];
  FIXP_DBL      sfbMinSnrLdData[MAX_GROUPED_SFB];
  FIXP_DBL      sfbEnergyLdData[MAX_GROUPED_SFB];
  FIXP_DBL      sfbEnergy[MAX_GROUPED_SFB];
  FIXP_DBL      sfbWeightedEnergyLdData[MAX_GROUPED_SFB];

  FIXP_DBL      sfbEnFacLd[MAX_GROUPED_SFB];

  FIXP_DBL      sfbSpreadEnergy[MAX_GROUPED_SFB];

} QC_OUT_CHANNEL;


typedef struct
{
  EXT_PAYLOAD_TYPE  type;  /* type of the extension payload */
  INT    nPayloadBits;     /* size of the payload */
  UCHAR *pPayload;         /* pointer to payload */

} QC_OUT_EXTENSION;


typedef struct
{
  INT          staticBitsUsed; /* for verification purposes */
  INT          dynBitsUsed;    /* for verification purposes */

  INT          extBitsUsed;    /* bit consumption of extended fill elements */
  INT          nExtensions;    /* number of extension payloads for this element */
  QC_OUT_EXTENSION extension[(1)];  /* reffering extension payload */

  INT          grantedDynBits;

  INT          grantedPe;
  INT          grantedPeCorr;

  PE_DATA      peData;

  QC_OUT_CHANNEL *qcOutChannel[(2)];


} QC_OUT_ELEMENT;

typedef struct
{
  QC_OUT_ELEMENT    *qcElement[(6)];
  QC_OUT_CHANNEL    *pQcOutChannels[(6)];
  QC_OUT_EXTENSION   extension[(2+2)];  /* global extension payload */
  INT          nExtensions;       /* number of extension payloads for this AU */
  INT          maxDynBits;        /* maximal allowed dynamic bits in frame */
  INT          grantedDynBits;    /* granted dynamic bits in frame */
  INT          totFillBits;       /* fill bits */
  INT          elementExtBits;    /* element associated extension payload bits, e.g. sbr, drc ... */
  INT          globalExtBits;     /* frame/au associated extension payload bits (anc data ...) */
  INT          staticBits;        /* aac side info bits */

  INT          totalNoRedPe;
  INT          totalGrantedPeCorr;

  INT          usedDynBits;       /* number of dynamic bits in use */
  INT          alignBits;         /* AU alignment bits */
  INT          totalBits;         /* sum of static, dyn, sbr, fill, align and dse bits */

} QC_OUT;

typedef struct {
  INT chBitrateEl;                    /* channel bitrate in element (totalbitrate*el_relativeBits/el_channels) */
  INT maxBitsEl;                      /* used in crash recovery */
  INT bitResLevelEl;                  /* update bitreservoir level in each call of FDKaacEnc_QCMain */
  INT maxBitResBitsEl;                /* nEffChannels*6144 - averageBitsInFrame */
  FIXP_DBL relativeBitsEl;            /* Bits relative to total Bits*/
} ELEMENT_BITS;

typedef struct
{
  /* this is basically struct QC_INIT */

  INT globHdrBits;
  INT maxBitsPerFrame;   /* maximal allowed bits per frame, 6144*nChannelsEff */
  INT minBitsPerFrame;   /* minimal allowd bits per fram, superframing - DRM */
  INT nElements;
  QCDATA_BR_MODE bitrateMode;
  INT bitDistributenMode; /* 0: full bitreservoir, 1: reduced bitreservoir, 2: disabled bitreservoir */
  INT bitResTot;
  INT bitResTotMax;
  INT maxIterations;      /* Maximum number of allowed iterations before FDKaacEnc_crashRecovery() is applied. */
  INT invQuant;

  FIXP_DBL vbrQualFactor;
  FIXP_DBL maxBitFac;

  PADDING padding;

  ELEMENT_BITS  *elementBits[(6)];
  BITCNTR_STATE *hBitCounter;
  ADJ_THR_STATE *hAdjThr;

} QC_STATE;

#endif /* _QC_DATA_H */




