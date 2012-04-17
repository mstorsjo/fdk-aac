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

  This module declares all static and dynamic memory spaces
*/

#include "sbr_ram.h"




#define WORKBUFFER1_TAG 0
#define WORKBUFFER2_TAG 1

/*!
  \name StaticSbrData

  Static memory areas, must not be overwritten in other sections of the decoder
*/
/* @{ */

/*! SBR Decoder main structure */
C_ALLOC_MEM(Ram_SbrDecoder, struct SBR_DECODER_INSTANCE, 1)
/*! SBR Decoder element data  <br>
  Dimension: (4) */
C_ALLOC_MEM2(Ram_SbrDecElement, SBR_DECODER_ELEMENT, 1, (4))
/*! SBR Decoder individual channel data  <br>
  Dimension: (6) */
C_ALLOC_MEM2(Ram_SbrDecChannel, SBR_CHANNEL, 1, (6)+1)

/*! Filter states for QMF-synthesis.  <br>
  Dimension: #(6) * (#QMF_FILTER_STATE_SYN_SIZE-#(64)) */
C_AALLOC_MEM2_L(Ram_sbr_QmfStatesSynthesis, FIXP_QSS, (640)-(64), (6)+1, SECT_DATA_L1)

/*! Delayed spectral data needed for the dynamic framing of SBR.
  For mp3PRO, 1/3 of a frame is buffered (#(6) 6) */
C_AALLOC_MEM2(Ram_sbr_OverlapBuffer, FIXP_DBL, 2 * (6) * (64), (6)+1)

/*! Static Data of PS */

C_ALLOC_MEM(Ram_ps_dec, PS_DEC, 1)


/* @} */


/*!
  \name DynamicSbrData

  Dynamic memory areas, might be reused in other algorithm sections,
  e.g. the core decoder
  <br>
  Depending on the mode set by DONT_USE_CORE_WORKBUFFER, workbuffers are
  defined additionally to the CoreWorkbuffer.
  <br>
  The size of WorkBuffers is ((1024)/(32))*(64) = 2048.
  <br>
  WorkBuffer2 is a pointer to the CoreWorkBuffer wich is reused here in the SBR part. In case of
  DONT_USE_CORE_WORKBUFFER, the CoreWorkbuffer is not used and the according
  Workbuffer2 is defined locally in this file.
  <br>
  WorkBuffer1 is reused in the AAC core (-> aacdecoder.cpp, aac_ram.cpp)
  <br>

  Use of WorkBuffers:
  <pre>

    -------------------------------------------------------------
    AAC core:

      CoreWorkbuffer: spectral coefficients
      WorkBuffer1:    CAacDecoderChannelInfo, CAacDecoderDynamicData

    -------------------------------------------------------------
    SBR part:
      ----------------------------------------------
      Low Power Mode (useLP=1 or LOW_POWER_SBR_ONLY), see assignLcTimeSlots()

        SLOT_BASED_PROTOTYPE_SYN_FILTER

        WorkBuffer1                                WorkBuffer2(=CoreWorkbuffer)
         ________________                           ________________
        | RealLeft       |                         | RealRight      |
        |________________|                         |________________|

      ----------------------------------------------
      High Quality Mode (!LOW_POWER_SBR_ONLY and useLP=0), see assignHqTimeSlots()

         SLOTBASED_PS

         WorkBuffer1                                WorkBuffer2(=CoreWorkbuffer)
         ________________                           ________________
        | Real/Imag      |  interleaved            | Real/Imag      |  interleaved
        |________________|  first half actual ch   |________________|  second half actual ch

    -------------------------------------------------------------

  </pre>

*/
/* @{ */
C_ALLOC_MEM_OVERLAY(Ram_SbrDecWorkBuffer1, FIXP_DBL, ((1024)/(32))*(64), SECT_DATA_L1, WORKBUFFER1_TAG)
C_ALLOC_MEM_OVERLAY(Ram_SbrDecWorkBuffer2, FIXP_DBL, ((1024)/(32))*(64), SECT_DATA_L2, WORKBUFFER2_TAG)

/* @} */




