/*****************************  MPEG-4 AAC Decoder  **************************

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


   $Id$
   Author(s):   Daniel Homm
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef LATM_DEMUX_H
#define LATM_DEMUX_H

#include "tpdec_lib.h"


#include "FDK_bitstream.h"

#define MIN_LATM_HEADERLENGTH  9
#define MIN_LOAS_HEADERLENGTH  MIN_LATM_HEADERLENGTH + 24   /* both in bits */
 #define MIN_TP_BUF_SIZE_LOAS   ( 8194 )

enum {
  LATM_MAX_PROG = 1,
  LATM_MAX_LAYER = 2,
  LATM_MAX_VAR_CHUNKS=16,
  LATM_MAX_ID=16
};

typedef struct {
  UINT m_frameLengthType;
  UINT m_bufferFullness;
  UINT m_streamID;
  UINT m_frameLengthInBits;
} LATM_LAYER_INFO;

typedef struct {
  LATM_LAYER_INFO m_linfo[LATM_MAX_PROG][LATM_MAX_LAYER];
  UINT m_taraBufferFullness;
  UINT m_otherDataLength;
  UINT m_audioMuxLengthBytes;          /* Length of LOAS payload */

  UCHAR m_useSameStreamMux;
  UCHAR m_AudioMuxVersion;
  UCHAR m_AudioMuxVersionA;
  UCHAR m_allStreamsSameTimeFraming;
  UCHAR m_noSubFrames;
  UCHAR m_numProgram;
  UCHAR m_numLayer;
  UCHAR m_useSameConfig;

  UCHAR m_otherDataPresent;
  UCHAR m_crcCheckPresent;
  UCHAR m_crcCheckSum;

  SCHAR BufferFullnessAchieved;
} CLatmDemux;

int CLatmDemux_ReadAuChunkLengthInfo(HANDLE_FDK_BITSTREAM bs);

TRANSPORTDEC_ERROR CLatmDemux_Read(
        HANDLE_FDK_BITSTREAM bs,
        CLatmDemux *pLatmDemux,
        TRANSPORT_TYPE tt,
        CSTpCallBacks *pTpDecCallbacks,
        CSAudioSpecificConfig *pAsc,
        const INT ignoreBufferFullness
        );

TRANSPORTDEC_ERROR CLatmDemux_ReadStreamMuxConfig(
        HANDLE_FDK_BITSTREAM bs,
        CLatmDemux *pLatmDemux,
        CSTpCallBacks *pTpDecCallbacks,
        CSAudioSpecificConfig *pAsc
        );

TRANSPORTDEC_ERROR CLatmDemux_ReadPayloadLengthInfo(HANDLE_FDK_BITSTREAM bs, CLatmDemux *pLatmDemux);

int  CLatmDemux_GetFrameLengthInBits(CLatmDemux *pLatmDemux);
int  CLatmDemux_GetOtherDataPresentFlag(CLatmDemux *pLatmDemux);
int  CLatmDemux_GetOtherDataLength(CLatmDemux *pLatmDemux);
UINT CLatmDemux_GetNrOfSubFrames(CLatmDemux *pLatmDemux);


#endif /* LATM_DEMUX_H */
