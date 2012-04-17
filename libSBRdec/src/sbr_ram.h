/****************************************************************************

                     (C) Copyright Fraunhofer IIS (2005)
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
\brief Memory layout
$Revision: 36841 $
*/
#ifndef _SBR_RAM_H_
#define _SBR_RAM_H_

#include "sbrdecoder.h"

#include "env_extr.h"
#include "sbr_dec.h"



#define SBRDEC_MAX_CH_PER_ELEMENT  (2)

typedef struct
{
  SBR_CHANNEL          *pSbrChannel[SBRDEC_MAX_CH_PER_ELEMENT];
  TRANSPOSER_SETTINGS   transposerSettings; /* Common transport settings for each individual channel of an element */
  HANDLE_FDK_BITSTREAM  hBs;

  MP4_ELEMENT_ID        elementID;          /* Element ID set during initialization. Can be used for concealment */
  int                   nChannels;          /* Number of elements output channels (=2 in case of PS) */

  UCHAR frameErrorFlag[(1)+1];  /* Frame error status (for every slot in the delay line).
                                                       Will be copied into header at the very beginning of decodeElement() routine. */

  UCHAR useFrameSlot;          /* Index which defines which slot will be decoded/filled next (used with additional delay) */
  UCHAR useHeaderSlot[(1)+1];   /* Index array that provides the link between header and frame data
                                                       (important when processing with additional delay). */
} SBR_DECODER_ELEMENT;


struct SBR_DECODER_INSTANCE
{
  SBR_DECODER_ELEMENT  *pSbrElement[(4)];
  SBR_HEADER_DATA       sbrHeader[(4)][(1)+1];      /* Sbr header for each individual channel of an element */

  FIXP_DBL *workBuffer1;
  FIXP_DBL *workBuffer2;

  HANDLE_PS_DEC  hParametricStereoDec;

  /* Global parameters */
  AUDIO_OBJECT_TYPE coreCodec; /* AOT of core codec */
  int numSbrElements;
  int numSbrChannels;
  INT sampleRateIn;            /* SBR decoder input sampling rate; might be different than the transposer input sampling rate. */
  INT sampleRateOut;           /* Sampling rate of the SBR decoder output audio samples. */
  USHORT codecFrameSize;
  UCHAR synDownsampleFac;
  UCHAR numDelayFrames;        /* The current number of additional delay frames used for processing. */

  UINT flags;

};

H_ALLOC_MEM(Ram_SbrDecElement, SBR_DECODER_ELEMENT)
H_ALLOC_MEM(Ram_SbrDecChannel, SBR_CHANNEL)
H_ALLOC_MEM(Ram_SbrDecoder, struct SBR_DECODER_INSTANCE)

H_ALLOC_MEM(Ram_sbr_QmfStatesSynthesis, FIXP_QSS)
H_ALLOC_MEM(Ram_sbr_OverlapBuffer, FIXP_DBL)


H_ALLOC_MEM(Ram_ps_dec, PS_DEC)


H_ALLOC_MEM_OVERLAY(Ram_SbrDecWorkBuffer1, FIXP_DBL)
H_ALLOC_MEM_OVERLAY(Ram_SbrDecWorkBuffer2, FIXP_DBL)


#endif /* _SBR_RAM_H_ */
