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
   Author(s):   Josef Hoepfl
   Description: ADTS interface

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "tpdec_adts.h"


#include "FDK_bitstream.h"



void adtsRead_CrcInit(HANDLE_ADTS pAdts) /*!< pointer to adts crc info stucture */
{
  FDKcrcInit(&pAdts->crcInfo, 0x8005, 0xFFFF, 16);
}

int adtsRead_CrcStartReg(
                     HANDLE_ADTS pAdts,          /*!< pointer to adts stucture */
                     HANDLE_FDK_BITSTREAM hBs,   /*!< handle to current bit buffer structure */
                     int mBits                   /*!< number of bits in crc region */
                   )
{
  if (pAdts->bs.protection_absent) {
    return 0;
  }

  return ( FDKcrcStartReg(&pAdts->crcInfo, hBs, mBits) );

}

void adtsRead_CrcEndReg(
                    HANDLE_ADTS pAdts, /*!< pointer to adts crc info stucture */
                    HANDLE_FDK_BITSTREAM hBs,   /*!< handle to current bit buffer structure */
                    int reg                    /*!< crc region */
                  )
{
  if (pAdts->bs.protection_absent == 0)
  {
    FDKcrcEndReg(&pAdts->crcInfo, hBs, reg);
  }
}

TRANSPORTDEC_ERROR adtsRead_CrcCheck( HANDLE_ADTS pAdts )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;
  USHORT crc;

  if (pAdts->bs.protection_absent)
    return TRANSPORTDEC_OK;

  crc = FDKcrcGetCRC(&pAdts->crcInfo);
  if (crc != pAdts->crcReadValue)
  {
    return (TRANSPORTDEC_CRC_ERROR);
  }

  return (ErrorStatus);
}



#define Adts_Length_SyncWord                     12
#define Adts_Length_Id                            1
#define Adts_Length_Layer                         2
#define Adts_Length_ProtectionAbsent              1
#define Adts_Length_Profile                       2
#define Adts_Length_SamplingFrequencyIndex        4
#define Adts_Length_PrivateBit                    1
#define Adts_Length_ChannelConfiguration          3
#define Adts_Length_OriginalCopy                  1
#define Adts_Length_Home                          1
#define Adts_Length_CopyrightIdentificationBit    1
#define Adts_Length_CopyrightIdentificationStart  1
#define Adts_Length_FrameLength                  13
#define Adts_Length_BufferFullness               11
#define Adts_Length_NumberOfRawDataBlocksInFrame  2
#define Adts_Length_CrcCheck                     16

TRANSPORTDEC_ERROR adtsRead_DecodeHeader(
        HANDLE_ADTS           pAdts,
        CSAudioSpecificConfig *pAsc,
        HANDLE_FDK_BITSTREAM  hBs,
        const INT             ignoreBufferFullness
        )
{
  INT crcReg;

  INT valBits;
  INT cmp_buffer_fullness;
  int i, adtsHeaderLength;

  STRUCT_ADTS_BS bs;

#ifdef TP_PCE_ENABLE
  CProgramConfig oldPce;
  /* Store the old PCE temporarily. Maybe we'll need it later if we
     have channelConfig=0 and no PCE in this frame. */
  FDKmemcpy(&oldPce, &pAsc->m_progrConfigElement, sizeof(CProgramConfig));
#endif

  valBits = FDKgetValidBits(hBs);

  /* adts_fixed_header */
  bs.mpeg_id           = FDKreadBits(hBs, Adts_Length_Id);
  bs.layer             = FDKreadBits(hBs, Adts_Length_Layer);
  bs.protection_absent = FDKreadBits(hBs, Adts_Length_ProtectionAbsent);
  bs.profile           = FDKreadBits(hBs, Adts_Length_Profile);
  bs.sample_freq_index = FDKreadBits(hBs, Adts_Length_SamplingFrequencyIndex);
  bs.private_bit       = FDKreadBits(hBs, Adts_Length_PrivateBit);
  bs.channel_config    = FDKreadBits(hBs, Adts_Length_ChannelConfiguration);
  bs.original          = FDKreadBits(hBs, Adts_Length_OriginalCopy);
  bs.home              = FDKreadBits(hBs, Adts_Length_Home);

  /* adts_variable_header */
  bs.copyright_id    = FDKreadBits(hBs, Adts_Length_CopyrightIdentificationBit);
  bs.copyright_start = FDKreadBits(hBs, Adts_Length_CopyrightIdentificationStart);
  bs.frame_length    = FDKreadBits(hBs, Adts_Length_FrameLength);
  bs.adts_fullness   = FDKreadBits(hBs, Adts_Length_BufferFullness);
  bs.num_raw_blocks  = FDKreadBits(hBs, Adts_Length_NumberOfRawDataBlocksInFrame);
  bs.num_pce_bits    = 0;

  adtsHeaderLength = ADTS_HEADERLENGTH;

  if (!bs.protection_absent) {
    FDKcrcReset(&pAdts->crcInfo);
    FDKpushBack(hBs, 56);   /* complete fixed and variable header! */
    crcReg = FDKcrcStartReg(&pAdts->crcInfo, hBs, 0);
    FDKpushFor(hBs, 56);
  }

  if (! bs.protection_absent && bs.num_raw_blocks>0) {
    for (i=0; i<bs.num_raw_blocks; i++) {
      pAdts->rawDataBlockDist[i] = (USHORT)FDKreadBits(hBs, 16);
      adtsHeaderLength += 16;
    }
    /* Change raw data blocks to delta values */
    pAdts->rawDataBlockDist[bs.num_raw_blocks] = bs.frame_length - 7 - bs.num_raw_blocks*2 - 2 ;
    for (i=bs.num_raw_blocks; i>0; i--) {
      pAdts->rawDataBlockDist[i] -= pAdts->rawDataBlockDist[i-1];
    }
  }

  /* adts_error_check */
  if (!bs.protection_absent)
  {
    USHORT crc_check;

    FDKcrcEndReg(&pAdts->crcInfo, hBs, crcReg);
    crc_check = FDKreadBits(hBs, Adts_Length_CrcCheck);
    adtsHeaderLength += Adts_Length_CrcCheck;

    pAdts->crcReadValue = crc_check;
    /* Check header CRC in case of multiple raw data blocks */
    if (bs.num_raw_blocks > 0) {
      if (pAdts->crcReadValue != FDKcrcGetCRC(&pAdts->crcInfo)) {
        return TRANSPORTDEC_CRC_ERROR;
      }
      /* Reset CRC for the upcoming raw_data_block() */
      FDKcrcReset(&pAdts->crcInfo);
    }
  }


  /* check if valid header */
  if (
      (bs.layer != 0) ||                          // we only support MPEG ADTS
      (bs.sample_freq_index >= 13)                // we only support 96kHz - 7350kHz
      ) {
    FDKpushFor(hBs, bs.frame_length * 8);   // try again one frame later
    return TRANSPORTDEC_UNSUPPORTED_FORMAT;
  }

  /* special treatment of id-bit */
  if ( (bs.mpeg_id == 0) && (pAdts->decoderCanDoMpeg4 == 0) )
  {
    /* MPEG-2 decoder cannot play MPEG-4 bitstreams */


    FDKpushFor(hBs, bs.frame_length * 8);  // try again one frame later
    return TRANSPORTDEC_UNSUPPORTED_FORMAT;
  }

  if (!ignoreBufferFullness)
  {
    cmp_buffer_fullness = bs.frame_length*8 + bs.adts_fullness*32*getNumberOfEffectiveChannels(bs.channel_config);


    /* Evaluate buffer fullness */
    if (bs.adts_fullness != 0x7FF)
    {
      if (pAdts->BufferFullnesStartFlag)
      {
        if ( valBits < cmp_buffer_fullness )
        {
          /* Condition for start of decoding is not fulfilled */

          /* The current frame will not be decoded */
          FDKpushBack(hBs, adtsHeaderLength);

          if ( (cmp_buffer_fullness+adtsHeaderLength) > ((TRANSPORTDEC_INBUF_SIZE<<3)-7) ) {
            return TRANSPORTDEC_SYNC_ERROR;
          } else {
            return TRANSPORTDEC_NOT_ENOUGH_BITS;
          }
        }
        else
        {
          pAdts->BufferFullnesStartFlag = 0;
        }
      }
    }
  }


  /* Get info from ADTS header */
  AudioSpecificConfig_Init(pAsc);
  pAsc->m_aot                    = (AUDIO_OBJECT_TYPE)(bs.profile + 1);
  pAsc->m_samplingFrequencyIndex = bs.sample_freq_index;
  pAsc->m_samplingFrequency      = SamplingRateTable[bs.sample_freq_index];
  pAsc->m_channelConfiguration   = bs.channel_config;
  pAsc->m_samplesPerFrame        = 1024;

#ifdef TP_PCE_ENABLE
  if (bs.channel_config == 0)
  {
    int pceBits = 0;
    UINT alignAnchor = FDKgetValidBits(hBs);
    
    if (FDKreadBits(hBs,3) == ID_PCE) {
      /* Got luck! Parse the PCE */
      int crcReg;
      crcReg = adtsRead_CrcStartReg(pAdts, hBs, 0);

      CProgramConfig_Read(&pAsc->m_progrConfigElement, hBs, alignAnchor);

      adtsRead_CrcEndReg(pAdts, hBs, crcReg);
      pceBits = alignAnchor - FDKgetValidBits(hBs);
      /* store the number of PCE bits */
      bs.num_pce_bits = pceBits;
    }
    else {
      /* No PCE in this frame! Push back the ID tag bits. */
      FDKpushBack(hBs,3);

      /* Encoders do not have to write a PCE in each frame.
         So if we already have a valid PCE we have to use it. */
      if (  oldPce.isValid
        && (bs.sample_freq_index == pAdts->bs.sample_freq_index)  /* we could compare the complete fixed header (bytes) here! */
        && (bs.channel_config    == pAdts->bs.channel_config) /* == 0 */
        && (bs.mpeg_id           == pAdts->bs.mpeg_id) )
      { /* Restore previous PCE which is still valid */
        FDKmemcpy(&pAsc->m_progrConfigElement, &oldPce, sizeof(CProgramConfig));
      }
      else if (bs.mpeg_id == 0) {
        /* If not it seems that we have a implicit channel configuration.
           This mode is not allowed in the context of ISO/IEC 14496-3.
           Skip this frame and try the next one. */
        FDKpushFor(hBs, (bs.frame_length<<3) - adtsHeaderLength - 3);
        return TRANSPORTDEC_UNSUPPORTED_FORMAT;
      }
      /* else {
         ISO/IEC 13818-7 implicit channel mapping is allowed.
         So just open the box of chocolates to see what we got.
      } */
    }
  }
#endif /* TP_PCE_ENABLE */

  /* Copy bit stream data struct to persistent memory now, once we passed all sanity checks above. */
  FDKmemcpy(&pAdts->bs, &bs, sizeof(STRUCT_ADTS_BS));

  return TRANSPORTDEC_OK;
}

int adtsRead_GetRawDataBlockLength(
        HANDLE_ADTS pAdts,
        INT         blockNum
        )
{
  int length;

  if (pAdts->bs.num_raw_blocks == 0) {
    length = (pAdts->bs.frame_length - 7) << 3;    /* aac_frame_length subtracted by the header size (7 bytes). */
    if (pAdts->bs.protection_absent == 0)
      length -= 16;                                                                   /* substract 16 bit CRC */
  } else {
    if (pAdts->bs.protection_absent) {
      length = -1; /* raw data block length is unknown */
    } else {
      if (blockNum < 0 || blockNum > 3) {
        return TRANSPORTDEC_INVALID_PARAMETER;
      }
      length = (pAdts->rawDataBlockDist[blockNum] << 3) - 16;
    }
  }
  if (blockNum == 0) {
    length -= pAdts->bs.num_pce_bits;
  }
  return length;
}


