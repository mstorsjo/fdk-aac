
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

� Copyright  1995 - 2012 Fraunhofer-Gesellschaft zur F�rderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/******************************** MPEG Audio Encoder **************************

   Initial author:       serge
   contents/description: DAB Transport Headers support

******************************************************************************/
#include <stdio.h>
#include "FDK_audio.h"
#include "tpenc_dab.h"


#include "tpenc_lib.h"
#include "tpenc_asc.h"

#include "common_fix.h"

int dabWrite_CrcStartReg(
                                 HANDLE_DAB pDab,          /*!< pointer to dab stucture */
                                 HANDLE_FDK_BITSTREAM hBs,   /*!< handle to current bit buffer structure */
                                 int mBits                   /*!< number of bits in crc region */
                                )
{
  //fprintf(stderr, "dabWrite_CrcStartReg(%p): bits in crc region=%d\n", hBs, mBits);
  return ( FDKcrcStartReg(&pDab->crcInfo2, hBs, mBits) );
}

void dabWrite_CrcEndReg(
                                HANDLE_DAB pDab, /*!< pointer to dab crc info stucture */
                                HANDLE_FDK_BITSTREAM hBs,   /*!< handle to current bit buffer structure */
                                int reg                    /*!< crc region */
                               )
{
    //fprintf(stderr, "dabWrite_CrcEndReg(%p): crc region=%d\n", hBs, reg);
    FDKcrcEndReg(&pDab->crcInfo2, hBs, reg);
}

int dabWrite_GetHeaderBits( HANDLE_DAB hDab )
{
  int bits = 0;

  if (hDab->currentBlock == 0) {
    /* Static and variable header bits */
    bits += 16; //header_firecode 16
    bits +=  8; //rfa=1, dac_rate=1, sbr_flag=1, aac_channel_mode=1, ps_flag=1, mpeg_surround_config=3
    bits += 12 * hDab->num_raw_blocks; //au_start[1...num_aus] 12 bit AU start position markers

    //4 byte alignment
    if (hDab->dac_rate == 0 || hDab->sbr_flag == 0)
    	bits+=4;
    //16sbr => 16 + 5 + 3 + 12*(2-1) => 36 => 40 bits 5
    //24sbr => 16 + 5 + 3 + 12*(3-1) => 48 ok         6
    //32sbr => 16 + 5 + 3 + 12*(4-1) => 60 => 64 bits 8
    //48sbr => 16 + 5 + 3 + 12*(6-1) => 84 => 88 bits 11
  }

  /* Add raw data block CRC bits. Not really part of the header, put they cause bit overhead to be accounted. */
  bits += 16;


  return bits;
}


int dabWrite_CountTotalBitDemandHeader( HANDLE_DAB hDab, unsigned int streamDataLength )
{
	//fprintf(stderr, "streamDataLength=%d (%d bytes)\n", streamDataLength, streamDataLength >> 3);
	return dabWrite_GetHeaderBits(hDab);
}


INT dabWrite_Init(HANDLE_DAB hDab, CODER_CONFIG *config)
{
  /* Sanity checks */
	if((int)config->aot > 4
    || (int)config->aot < 1 ) {
    return -1;
  }

  /* Sanity checks DAB-specific */
  if ( !(config->nSubFrames == 2 && config->samplingRate == 16000 && (config->flags & CC_SBR)) &&
	   !(config->nSubFrames == 3 && config->samplingRate == 24000 && (config->flags & CC_SBR)) &&
	   !(config->nSubFrames == 4 && config->samplingRate == 32000) &&
	   !(config->nSubFrames == 6 && config->samplingRate == 48000)) {
	  return -1;
  }

  hDab->dac_rate = 0;
  hDab->aac_channel_mode=0;
  hDab->sbr_flag = 0;
  hDab->ps_flag = 0;
  hDab->mpeg_surround_config=0;
  hDab->subchannels_num=config->bitRate/8000;


  if(config->samplingRate == 24000 || config->samplingRate == 48000)
	  hDab->dac_rate = 1;

  if (config->extAOT==AOT_SBR || config->extAOT == AOT_PS)
	  hDab->sbr_flag = 1;

  if(config->extAOT == AOT_PS)
	  hDab->ps_flag = 1;


  if(config->channelMode == MODE_2)
	  hDab->aac_channel_mode =  1;

  //fprintf(stderr, "hDab->dac_rate=%d\n", hDab->dac_rate);
  //fprintf(stderr, "hDab->sbr_flag=%d\n", hDab->sbr_flag);
  //fprintf(stderr, "hDab->ps_flag=%d\n", hDab->ps_flag);
  //fprintf(stderr, "hDab->aac_channel_mode=%d\n", hDab->aac_channel_mode);
  //fprintf(stderr, "hDab->subchannels_num=%d\n", hDab->subchannels_num);
  //fprintf(stderr, "cc->nSubFrames=%d\n", config->nSubFrames);

  hDab->num_raw_blocks=config->nSubFrames-1; /* 0 means 1 raw data block */

  FDKcrcInit(&hDab->crcInfo, 0x1021, 0xFFFF, 16);
  FDKcrcInit(&hDab->crcFire, 0x782d, 0, 16);
  FDKcrcInit(&hDab->crcInfo2, 0x8005, 0xFFFF, 16);

  hDab->currentBlock = 0;
  hDab->headerBits = dabWrite_GetHeaderBits(hDab);

  return 0;
}

int dabWrite_EncodeHeader(HANDLE_DAB hDab,
                                 HANDLE_FDK_BITSTREAM hBitStream,
                                 int buffer_fullness,
                                 int frame_length)
{
  INT crcIndex = 0;


  FDK_ASSERT(((frame_length+hDab->headerBits)/8)<0x2000);      /*13 bit*/
  FDK_ASSERT(buffer_fullness<0x800);    /* 11 bit   */

  FDKcrcReset(&hDab->crcInfo);


//  fprintf(stderr, "dabWrite_EncodeHeader() hDab->currentBlock=%d, frame_length=%d, buffer_fullness=%d\n",
//		  hDab->currentBlock, frame_length, buffer_fullness);

//  if (hDab->currentBlock == 0) {
//	//hDab->subFrameStartPrev=dabWrite_GetHeaderBits(hDab);
//	fprintf(stderr, "header bits[%d] [%d]\n", hDab->subFrameStartPrev, hDab->subFrameStartPrev >> 3);
//    FDKresetBitbuffer(hBitStream, BS_WRITER);
//  }

  //hDab->subFrameStartBit = FDKgetValidBits(hBitStream);
//  fprintf(stderr, "dabWrite_EncodeHeader() hDab->subFrameStartBit=%d [%d]\n", hDab->subFrameStartBit, hDab->subFrameStartBit >> 3);

  //hDab->subFrameStartBit = FDKgetValidBits(hBitStream);
  /* Skip new header if this is raw data block 1..n */
  if (hDab->currentBlock == 0)
  {
    FDKresetBitbuffer(hBitStream, BS_WRITER);
//    fprintf(stderr, "dabWrite_EncodeHeader() after FDKresetBitbuffer=%d [%d]\n", FDKgetValidBits(hBitStream), FDKgetValidBits(hBitStream) >> 3);

    /* fixed header */
    FDKwriteBits(hBitStream, 0, 16); //header_firecode
    FDKwriteBits(hBitStream, 0, 1); //rfa
    FDKwriteBits(hBitStream, hDab->dac_rate, 1);
    FDKwriteBits(hBitStream, hDab->sbr_flag, 1);
    FDKwriteBits(hBitStream, hDab->aac_channel_mode, 1);
    FDKwriteBits(hBitStream, hDab->ps_flag, 1);
    FDKwriteBits(hBitStream, hDab->mpeg_surround_config, 3);
    /* variable header */
    int i;
    for(i=0; i<hDab->num_raw_blocks; i++)
    	FDKwriteBits(hBitStream, 0, 12);
    /* padding */
    if (hDab->dac_rate == 0 || hDab->sbr_flag == 0) {
    	FDKwriteBits(hBitStream, 0, 4);
    }
  } /* End of DAB header */

  hDab->subFrameStartBit = FDKgetValidBits(hBitStream);
  FDK_ASSERT(FDKgetValidBits(hBitStream) % 8 == 0); //only aligned header

//  fprintf(stderr, "dabWrite_EncodeHeader() FDKgetValidBits(hBitStream)=%d [%d]\n", FDKgetValidBits(hBitStream), FDKgetValidBits(hBitStream) >> 3);
  return 0;
}

int dabWrite_writeExtensionFillPayload(HANDLE_FDK_BITSTREAM  hBitStream, int extPayloadBits)
{
#define EXT_TYPE_BITS         ( 4 )
#define DATA_EL_VERSION_BITS  ( 4 )
#define FILL_NIBBLE_BITS      ( 4 )

#define EXT_TYPE_BITS         ( 4 )
#define DATA_EL_VERSION_BITS  ( 4 )
#define FILL_NIBBLE_BITS      ( 4 )

	INT  extBitsUsed = 0;
	INT extPayloadType = EXT_FIL;
	//fprintf(stderr, "FDKaacEnc_writeExtensionPayload() extPayloadType=%d\n", extPayloadType);
	if (extPayloadBits >= EXT_TYPE_BITS)
	{
	  UCHAR  fillByte = 0x00;  /* for EXT_FIL and EXT_FILL_DATA */

	  if (hBitStream != NULL) {
		FDKwriteBits(hBitStream, extPayloadType, EXT_TYPE_BITS);
	  }
	  extBitsUsed += EXT_TYPE_BITS;

	  switch (extPayloadType) {
		case EXT_FILL_DATA:
		  fillByte = 0xA5;
		case EXT_FIL:
		default:
		  if (hBitStream != NULL) {
			int writeBits = extPayloadBits;
			FDKwriteBits(hBitStream, 0x00, FILL_NIBBLE_BITS);
			writeBits -= 8;  /* acount for the extension type and the fill nibble */
			while (writeBits >= 8) {
			  FDKwriteBits(hBitStream, fillByte, 8);
			  writeBits -= 8;
			}
		  }
		  extBitsUsed += FILL_NIBBLE_BITS + (extPayloadBits & ~0x7) - 8;
		  break;
	  }
	}

	return (extBitsUsed);
}

void dabWrite_FillRawDataBlock(HANDLE_FDK_BITSTREAM  hBitStream, int payloadBits)
{
	INT extBitsUsed = 0;
#define EL_ID_BITS              ( 3 )
#define FILL_EL_COUNT_BITS      ( 4 )
#define FILL_EL_ESC_COUNT_BITS  ( 8 )
#define MAX_FILL_DATA_BYTES     ( 269 )
    while (payloadBits >= (EL_ID_BITS + FILL_EL_COUNT_BITS)) {
      INT cnt, esc_count=-1, alignBits=7;

      payloadBits -= EL_ID_BITS + FILL_EL_COUNT_BITS;
      if (payloadBits >= 15*8) {
        payloadBits -= FILL_EL_ESC_COUNT_BITS;
        esc_count = 0;  /* write esc_count even if cnt becomes smaller 15 */
      }
      alignBits = 0;

      cnt = fixMin(MAX_FILL_DATA_BYTES, (payloadBits+alignBits)>>3);

      if (cnt >= 15) {
        esc_count = cnt - 15 + 1;
      }

      if (hBitStream != NULL) {
        /* write bitstream */
        FDKwriteBits(hBitStream, ID_FIL, EL_ID_BITS);
        if (esc_count >= 0) {
          FDKwriteBits(hBitStream, 15, FILL_EL_COUNT_BITS);
          FDKwriteBits(hBitStream, esc_count, FILL_EL_ESC_COUNT_BITS);
        } else {
          FDKwriteBits(hBitStream, cnt, FILL_EL_COUNT_BITS);
        }
      }

      extBitsUsed += EL_ID_BITS + FILL_EL_COUNT_BITS + ((esc_count>=0) ? FILL_EL_ESC_COUNT_BITS : 0);

      cnt = fixMin(cnt*8, payloadBits);  /* convert back to bits */
#if 0
      extBitsUsed += FDKaacEnc_writeExtensionPayload( hBitStream,
                                                      pExtension->type,
                                                      pExtension->pPayload,
                                                      cnt );
#else
      extBitsUsed += dabWrite_writeExtensionFillPayload(hBitStream, cnt);
#endif
      payloadBits -= cnt;
    }
}

void dabWrite_EndRawDataBlock(HANDLE_DAB hDab,
                          HANDLE_FDK_BITSTREAM hBs,
                          int *pBits)
{
    FDK_BITSTREAM bsWriter;
	INT crcIndex = 0;
	USHORT crcData;
	INT writeBits=0;
	INT writeBitsNonLastBlock=0;
	INT writeBitsLastBlock=0;
#if 1
    if (hDab->currentBlock == hDab->num_raw_blocks) {
    	//calculate byte-alignment before writing ID_FIL
    	if((FDKgetValidBits(hBs)+3) % 8){
    		writeBits = 8 - ((FDKgetValidBits(hBs)+3) % 8);
    	}

    	INT offset_end = hDab->subchannels_num*110*8 - 2*8 - 3;
    	writeBitsLastBlock = offset_end - FDKgetValidBits(hBs);
        dabWrite_FillRawDataBlock(hBs, writeBitsLastBlock);
		FDKsyncCache(hBs);
		//fprintf(stderr, "FIL-element written=%d\n", writeBitsLastBlock);
		writeBitsLastBlock=writeBits;
    }
#endif
	FDKwriteBits(hBs, 7, 3); //finalize AU: ID_END
	FDKsyncCache(hBs);
	//byte-align (if ID_FIL doesn't align it).
	if(FDKgetValidBits(hBs) % 8){
		writeBits = 8 - (FDKgetValidBits(hBs) % 8);
        FDKwriteBits(hBs, 0x00, writeBits);
		FDKsyncCache(hBs);
	}

	//fake-written bits alignment for last AU
	if (hDab->currentBlock == hDab->num_raw_blocks)
		writeBits=writeBitsLastBlock;

	INT frameLen = (FDKgetValidBits(hBs) - hDab->subFrameStartBit) >> 3;
	//fprintf(stderr, "frame=%d, offset writeBits=%d\n", frameLen, writeBits);

	FDK_ASSERT(FDKgetValidBits(hBs) % 8 == 0); //only aligned au's
    FDK_ASSERT(hDab->subchannels_num*110*8 >= FDKgetValidBits(hBs)+2*8); //don't overlap superframe

    FDKinitBitStream(&bsWriter, hBs->hBitBuf.Buffer, hBs->hBitBuf.bufSize, 0, BS_WRITER);
    FDKpushFor(&bsWriter, hDab->subFrameStartBit);
    FDKcrcReset(&hDab->crcInfo);
    hDab->crcIndex = FDKcrcStartReg(&hDab->crcInfo, &bsWriter, 0);
#if 0
    if (hDab->currentBlock == hDab->num_raw_blocks) {
		INT offset_size = hDab->subchannels_num*110*8 - 2*8 - FDKgetValidBits(hBs);
		//fprintf(stderr, "offset_size=%d\n", offset_size >> 3);
		FDKpushFor(hBs, offset_size);
    }
#endif

    FDKpushFor(&bsWriter, FDKgetValidBits(hBs) - hDab->subFrameStartBit);
	FDKcrcEndReg(&hDab->crcInfo,  &bsWriter, hDab->crcIndex);
	crcData = FDKcrcGetCRC(&hDab->crcInfo);
	//fprintf(stderr, "crcData = %04x\n", crcData);
    /* Write inverted CRC of current raw data block */
    FDKwriteBits(hBs, crcData ^ 0xffff, 16);
	FDKsyncCache(hBs);


    /* Write distance to current data block */
	if(hDab->currentBlock) {
		FDKinitBitStream(&bsWriter, hBs->hBitBuf.Buffer, hBs->hBitBuf.bufSize, 0, BS_WRITER);
	    FDKpushFor(&bsWriter, 24 + (hDab->currentBlock-1)*12);
		//fprintf(stderr, "FDKwriteBits() = %d\n", hDab->subFrameStartBit>>3);
		FDKwriteBits(&bsWriter, (hDab->subFrameStartBit>>3), 12);
		FDKsyncCache(&bsWriter);
	}

	/* Write FireCode */
	if (hDab->currentBlock == hDab->num_raw_blocks) {
		FDKinitBitStream(&bsWriter, hBs->hBitBuf.Buffer, hBs->hBitBuf.bufSize, 0, BS_WRITER);
		FDKpushFor(&bsWriter, 16);

		FDKcrcReset(&hDab->crcFire);
		crcIndex = FDKcrcStartReg(&hDab->crcFire, &bsWriter, 72);
		FDKpushFor(&bsWriter, 9*8); //9bytes
		FDKcrcEndReg(&hDab->crcFire, &bsWriter, crcIndex);

		crcData = FDKcrcGetCRC(&hDab->crcFire);
		//fprintf(stderr, "Firecode: %04x\n", crcData);

		FDKinitBitStream(&bsWriter, hBs->hBitBuf.Buffer, hBs->hBitBuf.bufSize, 0, BS_WRITER);
		FDKwriteBits(&bsWriter, crcData, 16);
		FDKsyncCache(&bsWriter);
	}

    if (hDab->currentBlock == 0)
    	*pBits += hDab->headerBits;
    else
        *pBits += 16;

    *pBits += writeBits + 3; //size: ID_END + alignment

    /* Correct *pBits to reflect the amount of bits of the current subframe */
    *pBits -= hDab->subFrameStartBit;
    /* Fixup CRC bits, since they come after each raw data block */

    hDab->currentBlock++;
    //fprintf(stderr, "dabWrite_EndRawDataBlock() *pBits=%d (%d)\n", *pBits, *pBits >> 3);
}

