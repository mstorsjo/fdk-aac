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
/*!
  \file
  \brief  Main SBR structs definitions $Revision: 37142 $
*/

#ifndef __SBR_H
#define __SBR_H

#include "fram_gen.h"
#include "bit_sbr.h"
#include "tran_det.h"
#include "code_env.h"
#include "env_est.h"
#include "cmondata.h"

#include "qmf.h"
#include "resampler.h"

#include "ton_corr.h"


/* SBR bitstream delay */
  #define DELAY_FRAMES 2

typedef struct {
    struct ENV_CHANNEL        hEnvChannel;
    //INT_PCM                  *pDSOutBuffer;            /**< Pointer to downsampled audio output of SBR encoder */
    DOWNSAMPLER               downSampler;

} SBR_CHANNEL;
typedef SBR_CHANNEL* HANDLE_SBR_CHANNEL;

typedef struct {
    HANDLE_SBR_CHANNEL        sbrChannel[2];
    QMF_FILTER_BANK          *hQmfAnalysis[2];
    SBR_CONFIG_DATA           sbrConfigData;
    SBR_HEADER_DATA           sbrHeaderData;
    SBR_BITSTREAM_DATA        sbrBitstreamData;
    COMMON_DATA               CmonData;
    INT                       dynXOverFreqDelay[5];    /**< to delay a frame (I don't like it that much that way - hrc) */
    SBR_ELEMENT_INFO          elInfo;

    UCHAR                     payloadDelayLine[1+DELAY_FRAMES][MAX_PAYLOAD_SIZE];
    UINT                      payloadDelayLineSize[1+DELAY_FRAMES];                 /* Sizes in bits */

} SBR_ELEMENT;
typedef SBR_ELEMENT* HANDLE_SBR_ELEMENT;

struct SBR_ENCODER
{
  HANDLE_SBR_ELEMENT   sbrElement[(6)];
  HANDLE_SBR_CHANNEL   pSbrChannel[(6)];
  QMF_FILTER_BANK      QmfAnalysis[(6)];
  DOWNSAMPLER          lfeDownSampler;
  int                  lfeChIdx;                 /* -1 default for no lfe, else assign channel index */
  int                  noElements;               /* Number of elements */
  int                  nChannels;                /* Total channel count across all elements. */
  int                  frameSize;                /* SBR framelength. */
  int                  bufferOffset;             /* Offset for SBR parameter extraction in time domain input buffer. */
  int                  downsampledOffset;        /* Offset of downsampled/mixed output for core encoder. */
  int                  downmixSize;              /* Size in samples of downsampled/mixed output for core encoder. */
  int                  fTimeDomainDownsampling;  /* Flag signalling time domain downsampling instead of QMF downsampling. */
  int                  nBitstrDelay;             /* Amount of SBR frames to be delayed in bitstream domain. */
  INT                  estimateBitrate;          /* estimate bitrate of SBR encoder */
  INT                  inputDataDelay;           /* delay caused by downsampler, in/out buffer at sbrEncoder_EncodeFrame */

  UCHAR* dynamicRam;
  UCHAR* pSBRdynamic_RAM;

  HANDLE_PARAMETRIC_STEREO  hParametricStereo;
  QMF_FILTER_BANK           qmfSynthesisPS;

  /* parameters describing allocation volume of present instance */
  INT                  maxElements;
  INT                  maxChannels;
  INT                  supportPS;

} ;


#endif /* __SBR_H */
