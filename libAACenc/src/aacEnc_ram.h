/******************************************************************************

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

   $Id:
   Initial authors:      M. Lohwasser, M. Gayer
   Contents/description:

******************************************************************************/
/*!
  \file
  \brief  Memory layout  $Revision: 36838 $
  \author Markus Lohwasser
*/

#ifndef AAC_ENC_RAM_H
#define AAC_ENC_RAM_H

#include "common_fix.h"

#include "aacenc.h"
#include "psy_data.h"
#include "interface.h"
#include "psy_main.h"
#include "bitenc.h"
#include "bit_cnt.h"
#include "psy_const.h"

  #define  OUTPUTBUFFER_SIZE    (8192)  /*!< Output buffer size has to be at least 6144 bits per channel (768 bytes). FDK bitbuffer implementation expects buffer of size 2^n. */


/*
  Moved AAC_ENC struct definition from aac_enc.cpp into aacEnc_ram.h to get size and respective
  static memory in aacEnc_ram.cpp.
  aac_enc.h is the outward visible header file and putting the struct into would cause necessity
  of additional visible header files outside library.
*/

/* define hBitstream size: max AAC framelength is 6144 bits/channel */
/*#define BUFFER_BITSTR_SIZE ((6400*(6)/bbWordSize)    +((bbWordSize - 1) / bbWordSize))*/

struct AAC_ENC {

  AACENC_CONFIG    *config;

  INT               ancillaryBitsPerFrame;              /* ancillary bits per frame calculated from ancillary rate */

  CHANNEL_MAPPING   channelMapping;

  QC_STATE         *qcKernel;
  QC_OUT           *qcOut[(1)];

  PSY_OUT          *psyOut[(1)];
  PSY_INTERNAL     *psyKernel;

  /* lifetime vars */

  CHANNEL_MODE encoderMode;
  INT bandwidth90dB;
  AACENC_BITRATE_MODE bitrateMode;

  INT    dontWriteAdif;      /* use: write ADIF header only before 1st frame */

  FIXP_DBL  *dynamic_RAM;


  INT maxChannels;           /* used while allocation */
  INT maxElements;
  INT maxFrames;

  AUDIO_OBJECT_TYPE aot;    /* AOT to be used while encoding.  */

} ;

#define maxSize(a,b) ( ((a)>(b)) ? (a) : (b) )

#define BIT_LOOK_UP_SIZE         ( sizeof(INT)*(MAX_SFB_LONG*(CODE_BOOK_ESC_NDX+1)) )
#define MERGE_GAIN_LOOK_UP_SIZE  ( sizeof(INT)*MAX_SFB_LONG )



/* Dynamic RAM - Allocation */
/*
 ++++++++++++++++++++++++++++++++++++++++++++
 |   P_BUF_0   |         P_BUF_1            |
 ++++++++++++++++++++++++++++++++++++++++++++
 |  QC_OUT_CH  |         PSY_DYN            |
 ++++++++++++++++++++++++++++++++++++++++++++
 |             | BitLookUp+MergeGainLookUp  |
 ++++++++++++++++++++++++++++++++++++++++++++
 |             |  Bitstream output buffer   |
 ++++++++++++++++++++++++++++++++++++++++++++
*/

#define BUF_SIZE_0       ( ALIGN_SIZE(sizeof(QC_OUT_CHANNEL)*(6)) )
#define BUF_SIZE_1       ( ALIGN_SIZE(maxSize(sizeof(PSY_DYNAMIC), \
                                      (BIT_LOOK_UP_SIZE+MERGE_GAIN_LOOK_UP_SIZE))) )

#define P_BUF_0          ( 0 )
#define P_BUF_1          ( P_BUF_0 + BUF_SIZE_0 )

#define AAC_ENC_DYN_RAM_SIZE  ( BUF_SIZE_0 + BUF_SIZE_1 )


  H_ALLOC_MEM (AACdynamic_RAM, FIXP_DBL)
/*
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
END - Dynamic RAM - Allocation */

/*
  See further Memory Allocation details in aacEnc_ram.cpp
*/
  H_ALLOC_MEM (Ram_aacEnc_AacEncoder, AAC_ENC)

  H_ALLOC_MEM (Ram_aacEnc_PsyElement, PSY_ELEMENT)

  H_ALLOC_MEM (Ram_aacEnc_PsyInternal, PSY_INTERNAL)
  H_ALLOC_MEM (Ram_aacEnc_PsyStatic, PSY_STATIC)
  H_ALLOC_MEM (Ram_aacEnc_PsyInputBuffer, INT_PCM)

  PSY_DYNAMIC *GetRam_aacEnc_PsyDynamic (int n, UCHAR* dynamic_RAM);
  H_ALLOC_MEM (Ram_bsOutbuffer, UCHAR)

  H_ALLOC_MEM (Ram_aacEnc_PsyOutChannel, PSY_OUT_CHANNEL)

  H_ALLOC_MEM (Ram_aacEnc_PsyOut, PSY_OUT)
  H_ALLOC_MEM (Ram_aacEnc_PsyOutElements, PSY_OUT_ELEMENT)

  H_ALLOC_MEM (Ram_aacEnc_QCstate, QC_STATE)
  H_ALLOC_MEM (Ram_aacEnc_AdjustThreshold, ADJ_THR_STATE)

  H_ALLOC_MEM (Ram_aacEnc_AdjThrStateElement, ATS_ELEMENT)
  H_ALLOC_MEM (Ram_aacEnc_ElementBits, ELEMENT_BITS)
  H_ALLOC_MEM (Ram_aacEnc_BitCntrState, BITCNTR_STATE)

  INT *GetRam_aacEnc_BitLookUp(int n, UCHAR* dynamic_RAM);
  INT *GetRam_aacEnc_MergeGainLookUp(int n, UCHAR* dynamic_RAM);
  QC_OUT_CHANNEL *GetRam_aacEnc_QCchannel (int n, UCHAR* dynamic_RAM);

  H_ALLOC_MEM (Ram_aacEnc_QCout, QC_OUT)
  H_ALLOC_MEM (Ram_aacEnc_QCelement, QC_OUT_ELEMENT)


#endif /* #ifndef AAC_ENC_RAM_H */

