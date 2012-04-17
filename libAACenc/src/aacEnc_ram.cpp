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

#include "aacEnc_ram.h"

  C_ALLOC_MEM (AACdynamic_RAM, FIXP_DBL, AAC_ENC_DYN_RAM_SIZE/sizeof(FIXP_DBL))

/*
  Static memory areas, must not be overwritten in other sections of the decoder !
*/

/*
 The structure AacEncoder contains all Encoder structures.
*/

C_ALLOC_MEM (Ram_aacEnc_AacEncoder,           AAC_ENC,          1)


/*
   The structure PSY_INTERNAl contains all psych configuration and data pointer.
   * PsyStatic holds last and current Psych data.
   * PsyInputBuffer contains time input. Signal is needed at the beginning of Psych.
     Memory can be reused after signal is in time domain.
   * PsyData contains spectral, nrg and threshold information. Necessary data are
     copied into PsyOut, so memory is available after leaving psych.
   * TnsData, ChaosMeasure, PnsData are temporarily necessary, e.g. use memory from
     PsyInputBuffer.
*/

C_ALLOC_MEM2 (Ram_aacEnc_PsyElement, PSY_ELEMENT, 1, (6))

C_ALLOC_MEM  (Ram_aacEnc_PsyInternal, PSY_INTERNAL, 1)
C_ALLOC_MEM2 (Ram_aacEnc_PsyStatic,   PSY_STATIC,   1, (6))

C_ALLOC_MEM2 (Ram_aacEnc_PsyInputBuffer, INT_PCM, MAX_INPUT_BUFFER_SIZE, (6))

  PSY_DYNAMIC *GetRam_aacEnc_PsyDynamic (int n, UCHAR* dynamic_RAM) {
      FDK_ASSERT(dynamic_RAM!=0);
      return ((PSY_DYNAMIC*) (dynamic_RAM + P_BUF_1 + n*sizeof(PSY_DYNAMIC)));
  }

  C_ALLOC_MEM (Ram_bsOutbuffer, UCHAR, OUTPUTBUFFER_SIZE)

/*
   The structure PSY_OUT holds all psychoaccoustic data needed
   in quantization module
*/
C_ALLOC_MEM2 (Ram_aacEnc_PsyOut,         PSY_OUT, 1, (1))

C_ALLOC_MEM2 (Ram_aacEnc_PsyOutElements, PSY_OUT_ELEMENT, 1, (1)*(6))
C_ALLOC_MEM2 (Ram_aacEnc_PsyOutChannel,  PSY_OUT_CHANNEL, 1, (1)*(6))


/*
   The structure QC_STATE contains preinitialized settings and quantizer structures.
   * AdjustThreshold structure contains element-wise settings.
   * ElementBits contains elemnt-wise bit consumption settings.
   * When CRC is active, lookup table is necessary for fast crc calculation.
   * Bitcounter contains buffer to find optimal codebooks and minimal bit consumption.
     Values are temporarily, so dynamic memory can be used.
*/

C_ALLOC_MEM (Ram_aacEnc_QCstate, QC_STATE, 1)
C_ALLOC_MEM (Ram_aacEnc_AdjustThreshold, ADJ_THR_STATE, 1)

C_ALLOC_MEM2 (Ram_aacEnc_AdjThrStateElement, ATS_ELEMENT, 1, (6))
C_ALLOC_MEM2 (Ram_aacEnc_ElementBits, ELEMENT_BITS, 1, (6))
C_ALLOC_MEM (Ram_aacEnc_BitCntrState, BITCNTR_STATE, 1)

  INT *GetRam_aacEnc_BitLookUp(int n, UCHAR* dynamic_RAM) {
    FDK_ASSERT(dynamic_RAM!=0);
    return ((INT*) (dynamic_RAM + P_BUF_1));
  }
  INT *GetRam_aacEnc_MergeGainLookUp(int n, UCHAR* dynamic_RAM) {
    FDK_ASSERT(dynamic_RAM!=0);
    return ((INT*) (dynamic_RAM + P_BUF_1 + sizeof(INT)*(MAX_SFB_LONG*(CODE_BOOK_ESC_NDX+1))));
  }


/*
   The structure QC_OUT contains settings and structures holding all necessary information
   needed in bitstreamwriter.
*/

C_ALLOC_MEM2 (Ram_aacEnc_QCout, QC_OUT, 1, (1))
C_ALLOC_MEM2 (Ram_aacEnc_QCelement, QC_OUT_ELEMENT, 1, (1)*(6))
  QC_OUT_CHANNEL *GetRam_aacEnc_QCchannel (int n, UCHAR* dynamic_RAM) {
      FDK_ASSERT(dynamic_RAM!=0);
      return ((QC_OUT_CHANNEL*) (dynamic_RAM + P_BUF_0 + n*sizeof(QC_OUT_CHANNEL)));
  }












