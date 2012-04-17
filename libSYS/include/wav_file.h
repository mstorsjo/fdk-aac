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

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

/** \file   wav_file.h
 *  \brief  Rudimentary WAVE file read/write support.
 *
 *  The WAVE file reader/writer is intented to be used in the codec's example
 *  framework for easily getting started with encoding/decoding. Therefore
 *  it serves mainly for helping quickly understand how a codec's API actually
 *  works.
 *  Being a WAVE file reader/writer with very basic functionality, it may not be
 *  able to read WAVE files that come with unusual configurations.
 *  Details on how to use the interface functions can be found in every
 *  (encoder/decoder) example framework.
 */

#ifndef __WAV_FILE_H__
#define __WAV_FILE_H__



#include "genericStds.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * RIFF WAVE file struct.
 * For details see WAVE file format documentation (for example at http://www.wotsit.org).
 */
typedef struct WAV_HEADER
{
  char  riffType[4];
  UINT   riffSize;
  char  waveType[4];
  char  formatType[4];
  UINT   formatSize;
  USHORT compressionCode;
  USHORT numChannels;
  UINT   sampleRate;
  UINT   bytesPerSecond;
  USHORT blockAlign;
  USHORT bitsPerSample;
  char  dataType[4];
  UINT   dataSize;
} WAV_HEADER;

struct WAV
{
  WAV_HEADER header;
  FDKFILE *fp;
};

typedef struct WAV *HANDLE_WAV;

/**
 * \brief  Open a WAV file handle for reading.
 *
 * \param pWav      Pointer to a memory location, where a WAV handle is returned.
 * \param filename  File name to be opened.
 *
 * \return  0 on success and non-zero on failure.
 */
INT WAV_InputOpen (HANDLE_WAV *pWav, const char *filename);

/**
 * \brief  Read samples from a WAVE file. The samples are automatically re-ordered to the
 *         native host endianess and scaled to full scale of the INT_PCM type, from
 *         whatever BPS the WAVE file had specified in its header data.
 *
 *  \param wav           Handle of WAV file.
 *  \param sampleBuffer  Pointer to store audio data.
 *  \param numSamples    Desired number of samples to read.
 *  \param nBufBits      Size in bit of each audio sample of sampleBuffer.
 *
 *  \return  Number of samples actually read.
 */
INT WAV_InputRead (HANDLE_WAV wav, void *sampleBuffer, UINT numSamples, int nBufBits);

/**
 * \brief       Close a WAV file reading handle.
 * \param pWav  Pointer to a WAV file reading handle.
 * \return      void
 */
void WAV_InputClose(HANDLE_WAV *pWav);

/**
 * \brief  Open WAV output/writer handle.
 *
 * \param pWav            Pointer to WAV handle to be returned.
 * \param outputFilename  File name of the file to be written to.
 * \param sampleRate      Desired samplerate of the resulting WAV file.
 * \param numChannels     Desired number of audio channels of the resulting WAV file.
 * \param bitsPerSample   Desired number of bits per audio sample of the resulting WAV file.
 *
 * \return  0: ok; -1: error
 */
INT WAV_OutputOpen(HANDLE_WAV *pWav, const char *outputFilename, INT sampleRate, INT numChannels, INT bitsPerSample);

/**
 * \brief  Write data to WAV file asociated to WAV handle.
 *
 * \param wav              Handle of WAV file
 * \param sampleBuffer     Pointer to audio samples, right justified integer values.
 * \param numberOfSamples  The number of individual audio sample valuesto be written.
 * \param nBufBits         Size in bits of each audio sample in sampleBuffer.
 * \param nSigBits         Amount of significant bits of each nBufBits in sampleBuffer.
 *
 * \return 0: ok; -1: error
 */
INT WAV_OutputWrite(HANDLE_WAV wav, void *sampleBuffer, UINT numberOfSamples, int nBufBits, int nSigBits);

/**
 * \brief       Close WAV output handle.
 * \param pWav  Pointer to WAV handle. *pWav is set to NULL.
 * \return      void
 */
void WAV_OutputClose(HANDLE_WAV *pWav);

#ifdef __cplusplus
}
#endif


#endif /* __WAV_FILE_H__ */
