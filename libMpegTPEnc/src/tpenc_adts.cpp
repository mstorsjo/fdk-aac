/******************************** MPEG Audio Encoder **************************

                       (C) copyright Fraunhofer-IIS (1999)
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
   Initial author:       Alex Groeschel
   contents/description: ADTS Transport Headers support

******************************************************************************/

#include "tpenc_adts.h"


#include "tpenc_lib.h"
#include "tpenc_asc.h"


int adtsWrite_CrcStartReg(
                                 HANDLE_ADTS pAdts,          /*!< pointer to adts stucture */
                                 HANDLE_FDK_BITSTREAM hBs,   /*!< handle to current bit buffer structure */
                                 int mBits                   /*!< number of bits in crc region */
                                )
{
  if (pAdts->protection_absent) {
    return 0;
  }
  return ( FDKcrcStartReg(&pAdts->crcInfo, hBs, mBits) );
}

void adtsWrite_CrcEndReg(
                                HANDLE_ADTS pAdts, /*!< pointer to adts crc info stucture */
                                HANDLE_FDK_BITSTREAM hBs,   /*!< handle to current bit buffer structure */
                                int reg                    /*!< crc region */
                               )
{
  if (pAdts->protection_absent == 0)
  {
    FDKcrcEndReg(&pAdts->crcInfo, hBs, reg);
  }
}

int adtsWrite_GetHeaderBits( HANDLE_ADTS hAdts )
{
  int bits = 0;

  if (hAdts->currentBlock == 0) {
    /* Static and variable header bits */
    bits = 56;
    if (!hAdts->protection_absent) {
      /* Add header/ single raw data block CRC bits */
      bits += 16;
      if (hAdts->num_raw_blocks>0) {
        /* Add bits of raw data block position markers */
        bits += (hAdts->num_raw_blocks)*16;
      }
    }
  }
  if (!hAdts->protection_absent && hAdts->num_raw_blocks>0) {
    /* Add raw data block CRC bits. Not really part of the header, put they cause bit overhead to be accounted. */
    bits += 16;
  }

  hAdts->headerBits = bits;

  return bits;
}

INT adtsWrite_Init(HANDLE_ADTS hAdts, CODER_CONFIG *config)
{
  /* Sanity checks */
  if ( config->nSubFrames < 1
    || config->nSubFrames > 4
    || (int)config->aot > 4
    || (int)config->aot < 1 ) {
    return -1;
  }

  /* fixed header */
  if (config->flags & CC_MPEG_ID) {
    hAdts->mpeg_id = 0; /* MPEG 4 */
  } else {
    hAdts->mpeg_id = 1; /* MPEG 2 */
  }
  hAdts->layer=0;
  hAdts->protection_absent = ! (config->flags & CC_PROTECTION);
  hAdts->profile = ((int)config->aot) - 1;
  hAdts->sample_freq_index = getSamplingRateIndex(config->samplingRate);
  hAdts->sample_freq = config->samplingRate;
  hAdts->private_bit=0;
  hAdts->channel_mode = config->channelMode;
  hAdts->original=0;
  hAdts->home=0;
  /* variable header */
  hAdts->copyright_id=0;
  hAdts->copyright_start=0;

  hAdts->num_raw_blocks=config->nSubFrames-1; /* 0 means 1 raw data block */

  FDKcrcInit(&hAdts->crcInfo, 0x8005, 0xFFFF, 16);

  hAdts->currentBlock = 0;


  return 0;
}

int adtsWrite_EncodeHeader(HANDLE_ADTS hAdts,
                                 HANDLE_FDK_BITSTREAM hBitStream,
                                 int buffer_fullness,
                                 int frame_length)
{
  INT crcIndex = 0;


  hAdts->headerBits = adtsWrite_GetHeaderBits(hAdts);

  FDK_ASSERT(((frame_length+hAdts->headerBits)/8)<0x2000);      /*13 bit*/
  FDK_ASSERT(buffer_fullness<0x800);    /* 11 bit   */

  if (!hAdts->protection_absent) {
    FDKcrcReset(&hAdts->crcInfo);
  }

  if (hAdts->currentBlock == 0) {
    FDKresetBitbuffer(hBitStream, BS_WRITER);
  }

  hAdts->subFrameStartBit = FDKgetValidBits(hBitStream);

  /* Skip new header if this is raw data block 1..n */
  if (hAdts->currentBlock == 0)
  {
    FDKresetBitbuffer(hBitStream, BS_WRITER);

    if (hAdts->num_raw_blocks == 0) {
      crcIndex = adtsWrite_CrcStartReg(hAdts, hBitStream, 0);
    }

    /* fixed header */
    FDKwriteBits(hBitStream, 0xFFF, 12);
    FDKwriteBits(hBitStream, hAdts->mpeg_id, 1);
    FDKwriteBits(hBitStream, hAdts->layer, 2);
    FDKwriteBits(hBitStream, hAdts->protection_absent, 1);
    FDKwriteBits(hBitStream, hAdts->profile, 2);
    FDKwriteBits(hBitStream, hAdts->sample_freq_index, 4);
    FDKwriteBits(hBitStream, hAdts->private_bit, 1);
    FDKwriteBits(hBitStream, getChannelConfig(hAdts->channel_mode), 3);
    FDKwriteBits(hBitStream, hAdts->original, 1);
    FDKwriteBits(hBitStream, hAdts->home, 1);
    /* variable header */
    FDKwriteBits(hBitStream, hAdts->copyright_id, 1);
    FDKwriteBits(hBitStream, hAdts->copyright_start, 1);
    FDKwriteBits(hBitStream, (frame_length + hAdts->headerBits)>>3, 13);
    FDKwriteBits(hBitStream, buffer_fullness, 11);
    FDKwriteBits(hBitStream, hAdts->num_raw_blocks, 2);

    if (!hAdts->protection_absent) {
      int i;

      /* End header CRC portion for single raw data block and write dummy zero values for unknown fields. */
      if (hAdts->num_raw_blocks == 0) {
        adtsWrite_CrcEndReg(hAdts, hBitStream, crcIndex);
      } else {
        for (i=0; i<hAdts->num_raw_blocks; i++) {
          FDKwriteBits(hBitStream, 0, 16);
        }
      }
      FDKwriteBits(hBitStream, 0, 16);
    }
  } /* End of ADTS header */

  return 0;
}

void adtsWrite_EndRawDataBlock(HANDLE_ADTS hAdts,
                          HANDLE_FDK_BITSTREAM hBs,
                          int *pBits)
{
  if (!hAdts->protection_absent) {
    FDK_BITSTREAM bsWriter;

    FDKinitBitStream(&bsWriter, hBs->hBitBuf.Buffer, hBs->hBitBuf.bufSize, 0, BS_WRITER);
    FDKpushFor(&bsWriter, 56);

    if (hAdts->num_raw_blocks == 0) {
      FDKwriteBits(&bsWriter, FDKcrcGetCRC(&hAdts->crcInfo), 16);
    } else {
      int distance;

      /* Write CRC of current raw data block */
      FDKwriteBits(hBs, FDKcrcGetCRC(&hAdts->crcInfo), 16);

      /* Write distance to current data block */
      if (hAdts->currentBlock < hAdts->num_raw_blocks) {
        FDKpushFor(&bsWriter, hAdts->currentBlock*16);
        distance = FDKgetValidBits(hBs) - (56 + (hAdts->num_raw_blocks)*16 + 16);
        FDKwriteBits(&bsWriter, distance>>3, 16);
      }
    }
    FDKsyncCache(&bsWriter);
  }

  /* Write total frame lenth for multiple raw data blocks and header CRC */
  if (hAdts->num_raw_blocks > 0 && hAdts->currentBlock == hAdts->num_raw_blocks) {
    FDK_BITSTREAM bsWriter;
    int crcIndex = 0;

    FDKinitBitStream(&bsWriter, hBs->hBitBuf.Buffer, hBs->hBitBuf.bufSize, 0, BS_WRITER);

    if (!hAdts->protection_absent) {
      FDKcrcReset(&hAdts->crcInfo);
      crcIndex = FDKcrcStartReg(&hAdts->crcInfo, &bsWriter, 0);
    }
    /* Write total frame length */
    FDKpushFor(&bsWriter, 56-28+2);
    FDKwriteBits(&bsWriter, FDKgetValidBits(hBs)>>3, 13);

    /* Write header CRC */
    if (!hAdts->protection_absent) {
      FDKpushFor(&bsWriter, 11+2 + (hAdts->num_raw_blocks)*16);
      FDKcrcEndReg(&hAdts->crcInfo, &bsWriter, crcIndex);
      FDKwriteBits(&bsWriter, FDKcrcGetCRC(&hAdts->crcInfo), 16);
    }
    FDKsyncCache(&bsWriter);
  }

  /* Correct *pBits to reflect the amount of bits of the current subframe */
  *pBits -= hAdts->subFrameStartBit;
  if (!hAdts->protection_absent && hAdts->num_raw_blocks > 0) {
    /* Fixup CRC bits, since they come after each raw data block */
    *pBits += 16;
  }
  hAdts->currentBlock++;
}

