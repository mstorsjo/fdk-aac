/**************************  Fraunhofer IIS FDK SysLib  **********************

                        (C) Copyright Fraunhofer IIS (2000)
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
   Author(s):   Eric Allamanche
   Description: a rudimentary wav file interface

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/



#include "wav_file.h"
#include "genericStds.h"


static INT_PCM ulaw2pcm (UCHAR ulawbyte);

/*!
 *
 *  \brief Read header from a WAVEfile. Host endianess is handled accordingly.
 *  \wav->fp filepointer of type FILE*.
 *  \wavinfo SWavInfo struct where the decoded header info is stored into.
 *  \return 0 on success and non-zero on failure.
 *
 */
INT WAV_InputOpen (HANDLE_WAV *pWav, const char *filename)
{
    HANDLE_WAV wav = (HANDLE_WAV)FDKcalloc(1, sizeof(struct WAV));
    INT offset;

    if (wav == NULL) {
      FDKprintfErr("WAV_InputOpen(): Unable to allocate WAV struct.\n");
      goto error;
    }

    wav->fp = FDKfopen(filename, "rb");
    if (wav->fp == NULL) {
      FDKprintfErr("WAV_InputOpen(): Unable to open wav file. %s\n", filename);
      goto error;
    }

    /* read RIFF-chunk */
    if (FDKfread(&(wav->header.riffType), 1, 4, wav->fp) != 4) {
      FDKprintfErr("WAV_InputOpen(): couldn't read RIFF_ID\n");
      goto error;  /* bad error "couldn't read RIFF_ID" */
    }
    if (FDKstrncmp("RIFF", wav->header.riffType, 4)) {
      FDKprintfErr("WAV_InputOpen(): RIFF descriptor not found.\n") ;
      goto error;
    }

    /* Read RIFF size. Ignored. */
    FDKfread_EL(&(wav->header.riffSize), 4, 1, wav->fp);

    /* read WAVE-chunk */
    if (FDKfread(&wav->header.waveType, 1, 4, wav->fp) !=4) {
      FDKprintfErr("WAV_InputOpen(): couldn't read format\n");
      goto error;  /* bad error "couldn't read format" */
    }
    if (FDKstrncmp("WAVE", wav->header.waveType, 4)) {
      FDKprintfErr("WAV_InputOpen(): WAVE chunk ID not found.\n") ;
      goto error;
    }

    /* read format-chunk */
    if (FDKfread(&(wav->header.formatType), 1, 4, wav->fp) != 4) {
      FDKprintfErr("WAV_InputOpen(): couldn't read format_ID\n");
      goto error;  /* bad error "couldn't read format_ID" */
    }
    if (FDKstrncmp("fmt", wav->header.formatType, 3)) {
      FDKprintfErr("WAV_InputOpen(): fmt chunk format not found.\n") ;
     goto error;
    }


    FDKfread_EL(&wav->header.formatSize, 4, 1, wav->fp);   /* should be 16 for PCM-format (uncompressed) */


    /* read  info */
    FDKfread_EL(&(wav->header.compressionCode), 2, 1, wav->fp);
    FDKfread_EL(&(wav->header.numChannels), 2, 1, wav->fp);
    FDKfread_EL(&(wav->header.sampleRate), 4, 1, wav->fp);
    FDKfread_EL(&(wav->header.bytesPerSecond), 4, 1, wav->fp);
    FDKfread_EL(&(wav->header.blockAlign), 2, 1, wav->fp);
    FDKfread_EL(&(wav->header.bitsPerSample), 2, 1, wav->fp);

    offset = wav->header.formatSize - 16;

    /* Wave format extensible */
    if (wav->header.compressionCode == 0xFFFE) {
      static const UCHAR guidPCM[16] = {
          0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
          0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
      };
      USHORT extraFormatBytes, validBitsPerSample;
      UINT channelMask;
      UCHAR guid[16];
      INT i;

      /* read extra bytes */
      FDKfread_EL(&(extraFormatBytes), 2, 1, wav->fp);
      offset -= 2;

      if (extraFormatBytes >= 22) {
        FDKfread_EL(&(validBitsPerSample), 2, 1, wav->fp);
        FDKfread_EL(&(channelMask), 4, 1, wav->fp);
        FDKfread_EL(&(guid), 16, 1, wav->fp);

        /* check for PCM GUID */
        for (i = 0; i < 16; i++) if (guid[i] != guidPCM[i]) break;
        if (i == 16) wav->header.compressionCode = 0x01;

        offset -= 22;
      }
    }

    /* Skip rest of fmt header if any. */
    for (;offset > 0; offset--) {
      FDKfread(&wav->header.formatSize, 1, 1, wav->fp);
    }

    do {
      /* Read data chunk ID */
      if (FDKfread(wav->header.dataType, 1, 4, wav->fp) != 4) {
        FDKprintfErr("WAV_InputOpen(): Unable to read data chunk ID.\n");
        FDKfree(wav);
        goto error;
      }

      /* Read chunk length. */
      FDKfread_EL(&offset, 4, 1, wav->fp);

      /* Check for data chunk signature. */
      if (FDKstrncmp("data", wav->header.dataType, 4) == 0) {
        wav->header.dataSize = offset;
        break;
      }
      /* Jump over non data chunk. */
      for (;offset > 0; offset--) {
        FDKfread(&(wav->header.dataSize), 1, 1, wav->fp);
      }
    } while (!FDKfeof(wav->fp));

    /* return success */
    *pWav = wav;
    return 0;

    /* Error path */
error:
    if (wav->fp) {
      FDKfclose(wav->fp);
      wav->fp = NULL;
    }

    if (wav) {
      FDKfree(wav);
    }

    *pWav = NULL;

    return -1;
}

/*!
 *
 *  \brief Read samples from a WAVEfile. The samples are automatically reorder to the native
 *    host endianess and scaled to full scale of the INT_PCM type, from whatever bps the WAVEfile
 *    had specified in its haader data.
 *
 *  \wav HANDLE_WAV of the wav file.
 *  \buffer Pointer to store read data.
 *  \numSamples Desired number of samples to read.
 *  \nBits sample size in bits to be used for the buffer
 *
 *  \return Number of samples actually read.
 *
 */

INT WAV_InputRead (HANDLE_WAV wav, void *buffer, UINT numSamples, int nBits)
{
  UINT result = 0 ;
  UINT i;
  SCHAR *bptr = (SCHAR*)buffer;
  LONG  *lptr = (LONG*)buffer;
  SHORT *sptr = (SHORT*)buffer;

  switch (wav->header.compressionCode)
  {
    case 0x01:  /* PCM uncompressed */
      if (nBits == wav->header.bitsPerSample) {
        result = FDKfread_EL(buffer, wav->header.bitsPerSample >> 3, numSamples, wav->fp) ;
      } else {
        result = 0;
        for (i=0; i<numSamples; i++)
        {
          LONG tmp = 0;
          result += FDKfread_EL(&tmp, wav->header.bitsPerSample >> 3, 1, wav->fp) ;

          /* Move read bits to lower bits of LONG. */
          if ( !IS_LITTLE_ENDIAN() && wav->header.bitsPerSample != 24 && wav->header.bitsPerSample < 32) {
            tmp >>= (32-wav->header.bitsPerSample);
          }

          /* Full scale */
          if (wav->header.bitsPerSample > nBits)
            tmp >>= (wav->header.bitsPerSample-nBits);
          else
            tmp <<= (nBits-wav->header.bitsPerSample);
            
          if (nBits == 8)
            *bptr++ = (SCHAR) tmp;
          if (nBits == 16)
            *sptr++ = (SHORT) tmp;
          if (nBits == 32)
            *lptr++ = (LONG) tmp;
        }
      }
      break;

    case 0x07:  /* u-Law compression */
      for (i=0; i<numSamples; i++) {
        result += FDKfread(&(bptr[i<<1]), 1, 1, wav->fp) ;
        sptr[i] = ulaw2pcm(bptr[i<<1]) ;
      }
      break ;

    default:
      FDKprintf("WAV_InputRead(): unsupported data-compression!!") ;
      break ;
  }
  return result ;
}

void WAV_InputClose(HANDLE_WAV *pWav)
{
  HANDLE_WAV wav = *pWav;

  if (wav != NULL) {
    if (wav->fp != NULL) {
       FDKfclose(wav->fp);
       wav->fp = NULL;
    }
    if (wav) {
       FDKfree(wav);
    }
  }
  *pWav = NULL;
}

/* conversion of u-law to linear coding */
static INT_PCM ulaw2pcm (UCHAR ulawbyte)
{
  static const INT exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 } ;
  INT sign, exponent, mantissa, sample ;

  ulawbyte = (UCHAR)~ulawbyte ;
  sign = (ulawbyte & 0x80) ;
  exponent = (ulawbyte >> 4) & 0x07 ;
  mantissa = ulawbyte & 0x0F ;

  sample = exp_lut[exponent] + (mantissa << (exponent + 3)) ;
  if (sign != 0)
    sample = -sample ;

  return (INT_PCM)sample ;
}

/************** Writer ***********************/

static UINT LittleEndian32(UINT v)
{
    if (IS_LITTLE_ENDIAN())
        return v ;
    else
        return (v & 0x000000FF) << 24 | (v & 0x0000FF00) <<  8 | (v & 0x00FF0000) >>  8 | (v & 0xFF000000) >> 24;
}

static SHORT LittleEndian16(SHORT v)
{
    if (IS_LITTLE_ENDIAN())
        return v;
    else
        return (SHORT)(((v << 8) & 0xFF00) | ((v >> 8) & 0x00FF));
}

static USHORT Unpack(USHORT v)
{
    if (IS_LITTLE_ENDIAN())
      return v;
    else
      return (SHORT)(((v << 8) & 0xFF00) | ((v >> 8) & 0x00FF));
}

/**
 * WAV_OutputOpen
 * \brief Open WAV output/writer handle
 * \param pWav pointer to WAV handle to be returned
 * \param sampleRate desired samplerate of the resulting WAV file
 * \param numChannels desired number of audio channels of the resulting WAV file
 * \param bitsPerSample desired number of bits per audio sample of the resulting WAV file
 *
 * \return value:   0: ok
 *                 -1: error
 */
INT WAV_OutputOpen(HANDLE_WAV *pWav, const char *outputFilename, INT sampleRate, INT numChannels, INT bitsPerSample)
{
  HANDLE_WAV wav = (HANDLE_WAV)FDKcalloc(1, sizeof(struct WAV));
  UINT size = 0;

  if (bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32)
  {
      FDKprintfErr("WAV_OutputOpen(): Invalid argument (bitsPerSample).\n");
      goto bail;
  }

  wav->fp = FDKfopen(outputFilename, "wb");
  if (wav->fp == NULL)
  {
      FDKprintfErr("WAV_OutputOpen(): unable to create file %s\n", outputFilename);
      goto bail;
  }

  FDKstrcpy(wav->header.riffType, "RIFF");
  wav->header.riffSize = LittleEndian32(0x7fffffff);  /* in case fseek() doesn't work later in WAV_OutputClose() */
  FDKstrcpy(wav->header.waveType, "WAVE");

  FDKstrcpy(wav->header.formatType, "fmt ");
  wav->header.formatSize = LittleEndian32(16);

  wav->header.compressionCode = LittleEndian16(0x01);
  wav->header.bitsPerSample   = LittleEndian16((SHORT)bitsPerSample);
  wav->header.numChannels     = LittleEndian16((SHORT)numChannels);
  wav->header.blockAlign      = LittleEndian16((SHORT)(numChannels * (bitsPerSample >> 3)));
  wav->header.sampleRate      = LittleEndian32(sampleRate);
  wav->header.bytesPerSecond  = LittleEndian32(sampleRate * wav->header.blockAlign);
  FDKstrcpy(wav->header.dataType, "data");
  wav->header.dataSize        = LittleEndian32(0x7fffffff - 36);


  size = sizeof(WAV_HEADER);
  if (FDKfwrite(&wav->header, 1, size, wav->fp) != size)
  {
      FDKprintfErr("WAV_OutputOpen(): error writing to output file %s\n", outputFilename);
      goto bail;
  }


  wav->header.dataSize = wav->header.riffSize = 0;

  *pWav = wav;

  return 0;

bail:
  if (wav->fp)
    FDKfclose(wav->fp);
  if (wav)
    FDKfree(wav);

  pWav = NULL;

  return -1;
}


/**
 * WAV_OutputWrite
 * \brief Write data to WAV file asociated to WAV handle
 *
 * \param wav handle of wave file
 * \param sampleBuffer pointer to audio samples, right justified integer values
 * \param nBufBits size in bits of each audio sample in sampleBuffer
 * \param nSigBits amount of significant bits of each nBufBits in sampleBuffer
 *
 * \return value:    0: ok
 *                  -1: error
 */
INT WAV_OutputWrite(HANDLE_WAV wav, void *sampleBuffer, UINT numberOfSamples, int nBufBits, int nSigBits)
{
    SCHAR *bptr = (SCHAR*)sampleBuffer;
    SHORT *sptr = (SHORT*)sampleBuffer;
    LONG  *lptr = (LONG*)sampleBuffer;
    LONG tmp;
    
    int bps = Unpack(wav->header.bitsPerSample);
    UINT i;

    /* Pack samples if required */
    if (bps == nBufBits && bps == nSigBits) {
      if (FDKfwrite_EL(sampleBuffer, (bps>>3), numberOfSamples, wav->fp) != numberOfSamples)
      {
        FDKprintfErr("WAV_OutputWrite(): error: unable to write to file %d\n", wav->fp);
        return -1;
      }
    } else  {
      for (i=0; i<numberOfSamples; i++)
      {
        int result;
        int shift;
        
        switch (nBufBits) {
          case 8:  tmp = *bptr++; break;
          case 16: tmp = *sptr++; break;
          case 32: tmp = *lptr++; break;
          default: return -1;
        }
        /* Adapt sample size */
        shift = (nBufBits-nSigBits)-(32-bps);

        /* Correct alignment difference between 32 bit data buffer "tmp" and 24 bits to be written. */
        if ( !IS_LITTLE_ENDIAN() && bps == 24) {
          shift += 8;
        }

        if (shift < 0)
          tmp >>= -shift;
        else
          tmp <<= shift;

        /* Write sample */        
        result=FDKfwrite_EL(&tmp, bps>>3, 1, wav->fp);
        if (result <= 0) {
          FDKprintfErr("WAV_OutputWrite(): error: unable to write to file %d\n", wav->fp);
          return -1;
        }
      }
    } 

    wav->header.dataSize += (numberOfSamples * (bps>>3));
    return 0;
}


/**
 * WAV_OutputClose
 * \brief Close WAV Output handle
 * \param pWav pointer to WAV handle. *pWav is set to NULL.
 */
void WAV_OutputClose(HANDLE_WAV *pWav)
{
  HANDLE_WAV wav = *pWav;
  UINT size = 0;

  if ( wav == NULL ) {
      return;
  }

  wav->header.dataSize = LittleEndian32(wav->header.dataSize);
  wav->header.riffSize = LittleEndian32(wav->header.dataSize + 36);

  if (wav->fp != NULL)
  {
    if (FDKfseek(wav->fp, 0, FDKSEEK_SET)) {
       FDKprintf("WAV_OutputClose(): fseek() failed.\n");
    }

  size = sizeof(WAV_HEADER);
    if (FDKfwrite(&wav->header.riffType, 1, size, wav->fp) != size)
    {
        FDKprintfErr("WAV_OutputClose(): unable to write header\n");
	}

    if (FDKfclose(wav->fp))
    {
        FDKprintfErr("WAV_OutputClose(): unable to close wav file\n");
    }
    wav->fp = NULL;
  }

  FDKfree(wav);
  *pWav = NULL;
}

