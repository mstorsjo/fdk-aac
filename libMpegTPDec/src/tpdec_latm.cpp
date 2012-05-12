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

#include "tpdec_latm.h"


#include "FDK_bitstream.h"


#define TPDEC_TRACKINDEX(p,l) (2*(p) + (l))

static
UINT CLatmDemux_GetValue(HANDLE_FDK_BITSTREAM bs)
{
  UCHAR bytesForValue = 0, tmp = 0;
  int value = 0;

  bytesForValue = (UCHAR) FDKreadBits(bs,2);

  for (UINT i=0; i<=bytesForValue; i++) {
    value <<= 8;
    tmp = (UCHAR) FDKreadBits(bs,8);
    value += tmp;
  }

  return value;
}


static
TRANSPORTDEC_ERROR CLatmDemux_ReadAudioMuxElement(
        HANDLE_FDK_BITSTREAM bs,
        CLatmDemux *pLatmDemux,
        int m_muxConfigPresent,
        CSTpCallBacks *pTpDecCallbacks,
        CSAudioSpecificConfig *pAsc
        )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;

  if (m_muxConfigPresent) {
    pLatmDemux->m_useSameStreamMux = FDKreadBits(bs,1);

    if (!pLatmDemux->m_useSameStreamMux) {
      if ((ErrorStatus = CLatmDemux_ReadStreamMuxConfig(bs, pLatmDemux, pTpDecCallbacks, pAsc))) {
        return (ErrorStatus);
      }
    }
  }

  if (pLatmDemux->m_AudioMuxVersionA == 0) {
    /* Do only once per call, because parsing and decoding is done in-line. */
    if ((ErrorStatus = CLatmDemux_ReadPayloadLengthInfo(bs,pLatmDemux))) {
      return (ErrorStatus);
    }
  } else {
    /* audioMuxVersionA > 0 is reserved for future extensions */
    ErrorStatus = TRANSPORTDEC_UNSUPPORTED_FORMAT;
  }

  return (ErrorStatus);
}

TRANSPORTDEC_ERROR CLatmDemux_Read(
        HANDLE_FDK_BITSTREAM bs,
        CLatmDemux *pLatmDemux,
        TRANSPORT_TYPE tt,
        CSTpCallBacks *pTpDecCallbacks,
        CSAudioSpecificConfig *pAsc,
        const INT ignoreBufferFullness
        )
{
  UINT cntBits;
  UINT cmpBufferFullness;
  UINT audioMuxLengthBytesLast = 0;
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;

  cntBits = FDKgetValidBits(bs);

  if ((INT)cntBits < MIN_LATM_HEADERLENGTH) {
    return TRANSPORTDEC_NOT_ENOUGH_BITS;
  }

  if ((ErrorStatus = CLatmDemux_ReadAudioMuxElement(bs, pLatmDemux, (tt != TT_MP4_LATM_MCP0), pTpDecCallbacks, pAsc)))
    return (ErrorStatus);

  if (!ignoreBufferFullness)
  {
    cmpBufferFullness =   24+audioMuxLengthBytesLast*8
                        + pLatmDemux->m_linfo[0][0].m_bufferFullness* pAsc[TPDEC_TRACKINDEX(0,0)].m_channelConfiguration*32;

    /* evaluate buffer fullness */

    if (pLatmDemux->m_linfo[0][0].m_bufferFullness != 0xFF)
    {
      if (!pLatmDemux->BufferFullnessAchieved)
      {
        if (cntBits < cmpBufferFullness)
        {
          /* condition for start of decoding is not fulfilled */

          /* the current frame will not be decoded */
          return TRANSPORTDEC_NOT_ENOUGH_BITS;
        }
        else
        {
          pLatmDemux->BufferFullnessAchieved = 1;
        }
      }
    }
  }

  return (ErrorStatus);
}


TRANSPORTDEC_ERROR CLatmDemux_ReadStreamMuxConfig(
        HANDLE_FDK_BITSTREAM bs,
        CLatmDemux *pLatmDemux,
        CSTpCallBacks *pTpDecCallbacks,
        CSAudioSpecificConfig *pAsc
        )
{
  LATM_LAYER_INFO *p_linfo = NULL;
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;

  pLatmDemux->m_AudioMuxVersion = FDKreadBits(bs,1);

  if (pLatmDemux->m_AudioMuxVersion == 0) {
    pLatmDemux->m_AudioMuxVersionA = 0;
  } else {
    pLatmDemux->m_AudioMuxVersionA = FDKreadBits(bs,1);
  }

  if (pLatmDemux->m_AudioMuxVersionA == 0) {
    if (pLatmDemux->m_AudioMuxVersion == 1) {
      pLatmDemux->m_taraBufferFullness = CLatmDemux_GetValue(bs);
    }
    pLatmDemux->m_allStreamsSameTimeFraming = FDKreadBits(bs,1);
    pLatmDemux->m_noSubFrames = FDKreadBits(bs,6) + 1;
    pLatmDemux->m_numProgram  = FDKreadBits(bs,4) + 1;

    if (pLatmDemux->m_numProgram > 1) {
      return TRANSPORTDEC_UNSUPPORTED_FORMAT;
    }

    int idCnt = 0;
    for (UINT prog = 0; prog < pLatmDemux->m_numProgram; prog++) {
      pLatmDemux->m_numLayer = FDKreadBits(bs,3) + 1;
      if (pLatmDemux->m_numLayer > 2) {
        return TRANSPORTDEC_UNSUPPORTED_FORMAT;
      }

      for (UINT lay = 0; lay < pLatmDemux->m_numLayer; lay++) {
        p_linfo = &pLatmDemux->m_linfo[prog][lay];

        p_linfo->m_streamID = idCnt++;
        p_linfo->m_frameLengthInBits = 0;

        if( (prog == 0) && (lay == 0) ) {
          pLatmDemux->m_useSameConfig = 0;
        } else {
          pLatmDemux->m_useSameConfig = FDKreadBits(bs,1);
        }

        if (pLatmDemux->m_useSameConfig) {
          if (lay > 1) {
            FDKmemcpy(&pAsc[TPDEC_TRACKINDEX(prog,lay)], &pAsc[TPDEC_TRACKINDEX(prog,lay-1)], sizeof(CSAudioSpecificConfig));
          } else {
            return TRANSPORTDEC_PARSE_ERROR;
          }
        } else {
          if (pLatmDemux->m_AudioMuxVersion == 1)
          {
            FDK_BITSTREAM tmpBs;
            UINT ascStartPos, ascLen=0;

            ascLen = CLatmDemux_GetValue(bs);
            ascStartPos = FDKgetValidBits(bs);
            tmpBs = *bs;
            FDKsyncCache(&tmpBs);
            tmpBs.hBitBuf.ValidBits = ascLen;
            
            /* Read ASC */
            if ((ErrorStatus = AudioSpecificConfig_Parse(&pAsc[TPDEC_TRACKINDEX(prog,lay)], &tmpBs, 1, pTpDecCallbacks))) {
              return (ErrorStatus);
            }

            /* The field p_linfo->m_ascLen could be wrong, so check if */
            if ( 0 > (INT)FDKgetValidBits(&tmpBs)) {
              return TRANSPORTDEC_PARSE_ERROR;
            }
            FDKpushFor(bs, ascLen); /* position bitstream after ASC */
          }
          else {
            /* Read ASC */
            if ((ErrorStatus = AudioSpecificConfig_Parse(&pAsc[TPDEC_TRACKINDEX(prog,lay)], bs, 0, pTpDecCallbacks))) {
              return (ErrorStatus);
            }
          }
          {
            int cbError;

            cbError = pTpDecCallbacks->cbUpdateConfig(pTpDecCallbacks->cbUpdateConfigData, &pAsc[TPDEC_TRACKINDEX(prog,lay)]);
            if (cbError != 0) {
              return TRANSPORTDEC_UNKOWN_ERROR;
            }
          }
        }

        p_linfo->m_frameLengthType = FDKreadBits(bs,3);
        switch( p_linfo->m_frameLengthType ) {
        case 0:
          p_linfo->m_bufferFullness = FDKreadBits(bs,8);

          if (!pLatmDemux->m_allStreamsSameTimeFraming) {
            if ((lay > 0) && (pAsc[TPDEC_TRACKINDEX(prog,lay)].m_aot == AOT_AAC_SCAL || pAsc[TPDEC_TRACKINDEX(prog,lay)].m_aot == AOT_ER_AAC_SCAL)) {
              return TRANSPORTDEC_UNSUPPORTED_FORMAT;
            }
          }
          break;
        case 1:
          /* frameLength = FDKreadBits(bs,9); */
        case 3:
        case 4:
        case 5:
          /* CELP */
        case 6:
        case 7:
          /* HVXC */
        default:
          return TRANSPORTDEC_PARSE_ERROR; //_LATM_INVALIDFRAMELENGTHTYPE;

        }  /* switch framelengthtype*/

      }  /* layer loop */
    }  /* prog loop */

    pLatmDemux->m_otherDataPresent = FDKreadBits(bs,1);
    pLatmDemux->m_otherDataLength  = 0;

    if (pLatmDemux->m_otherDataPresent) {
      int otherDataLenEsc  = 0;
      do {
        pLatmDemux->m_otherDataLength <<= 8;   // *= 256
        otherDataLenEsc = FDKreadBits(bs,1);
        pLatmDemux->m_otherDataLength += FDKreadBits(bs,8);
      } while (otherDataLenEsc);
    }

    pLatmDemux->m_crcCheckPresent = FDKreadBits(bs,1);
    pLatmDemux->m_crcCheckSum     = 0;

    if (pLatmDemux->m_crcCheckPresent) {
      pLatmDemux->m_crcCheckSum = FDKreadBits(bs,8);
    }

  }
  else {
    /* audioMuxVersionA > 0 is reserved for future extensions */
    ErrorStatus = TRANSPORTDEC_UNSUPPORTED_FORMAT;
  }
  return (ErrorStatus);
}

TRANSPORTDEC_ERROR CLatmDemux_ReadPayloadLengthInfo(HANDLE_FDK_BITSTREAM bs, CLatmDemux *pLatmDemux)
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;
  int totalPayloadBits = 0;

  if( pLatmDemux->m_allStreamsSameTimeFraming == 1 ) {
    for (UINT prog=0; prog<pLatmDemux->m_numProgram; prog++ ) {
      for (UINT lay=0; lay<pLatmDemux->m_numLayer; lay++ ) {
        LATM_LAYER_INFO *p_linfo = &pLatmDemux->m_linfo[prog][lay];

        switch (p_linfo->m_frameLengthType ) {
        case 0:
          p_linfo->m_frameLengthInBits = CLatmDemux_ReadAuChunkLengthInfo(bs);
          totalPayloadBits += p_linfo->m_frameLengthInBits;
          break;
        case 3:
        case 5:
        case 7:
        default:
          return TRANSPORTDEC_PARSE_ERROR; //AAC_DEC_LATM_INVALIDFRAMELENGTHTYPE;
        }
      }
    }
  }
  else {
    ErrorStatus = TRANSPORTDEC_PARSE_ERROR; //AAC_DEC_LATM_TIMEFRAMING;
  }
  if (pLatmDemux->m_audioMuxLengthBytes > 0 && totalPayloadBits > pLatmDemux->m_audioMuxLengthBytes*8) {
    return TRANSPORTDEC_PARSE_ERROR;
  }
  return (ErrorStatus);
}

int CLatmDemux_ReadAuChunkLengthInfo(HANDLE_FDK_BITSTREAM bs)
{
  UCHAR endFlag;
  int len = 0;

  do {
    UCHAR tmp = (UCHAR) FDKreadBits(bs,8);
    endFlag = (tmp < 255);

    len += tmp;

  } while( endFlag == 0 );

  len <<= 3;  /* convert from bytes to bits */

  return len;
}

int CLatmDemux_GetFrameLengthInBits(CLatmDemux *pLatmDemux)
{
  return pLatmDemux->m_linfo[0][0].m_frameLengthInBits;
}

int CLatmDemux_GetOtherDataPresentFlag(CLatmDemux *pLatmDemux)
{
  return pLatmDemux->m_otherDataPresent ? 1 : 0;
}

int CLatmDemux_GetOtherDataLength(CLatmDemux *pLatmDemux)
{
  return pLatmDemux->m_otherDataLength;
}

UINT CLatmDemux_GetNrOfSubFrames(CLatmDemux *pLatmDemux)
{
  return pLatmDemux->m_noSubFrames;
}

