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

#ifndef TPDEC_ADTS_H
#define TPDEC_ADTS_H

#include "tpdec_lib.h"


#define ADTS_SYNCWORD               ( 0xfff )
#define ADTS_SYNCLENGTH             ( 12 )     /* in bits */
#define ADTS_HEADERLENGTH           ( 56 )     /* minimum header size in bits */
#define ADTS_FIXED_HEADERLENGTH     ( 28 )     /* in bits */
#define ADTS_VARIABLE_HEADERLENGTH  ( ADTS_HEADERLENGTH - ADTS_FIXED_HEADERLENGTH )

#ifdef CHECK_TWO_SYNCS
 #define ADTS_MIN_TP_BUF_SIZE       ( 8191 + 2 )
#else
 #define ADTS_MIN_TP_BUF_SIZE       ( 8191 )
#endif

#include "FDK_crc.h"

typedef struct {
  /* ADTS header fields */
  UCHAR mpeg_id;
  UCHAR layer;
  UCHAR protection_absent;
  UCHAR profile;
  UCHAR sample_freq_index;
  UCHAR private_bit;
  UCHAR channel_config;
  UCHAR original;
  UCHAR home;
  UCHAR copyright_id;
  UCHAR copyright_start;
  USHORT frame_length;
  USHORT adts_fullness;
  UCHAR num_raw_blocks;
  UCHAR num_pce_bits;
} STRUCT_ADTS_BS;

struct STRUCT_ADTS {

  STRUCT_ADTS_BS bs;

  UCHAR decoderCanDoMpeg4;
  UCHAR BufferFullnesStartFlag;

  FDK_CRCINFO crcInfo;             /* CRC state info */
  USHORT      crcReadValue;        /* CRC value read from bitstream data */
  USHORT      rawDataBlockDist[4]; /* distance between each raw data block. Not the same as found in the bitstream */
} ;

typedef struct STRUCT_ADTS *HANDLE_ADTS;

/*!
  \brief Initialize ADTS CRC

  The function initialzes the crc buffer and the crc lookup table.

  \return  none
*/
void adtsRead_CrcInit( HANDLE_ADTS pAdts );

/**
 * \brief Starts CRC region with a maximum number of bits
 *        If mBits is positive zero padding will be used for CRC calculation, if there
 *        are less than mBits bits available.
 *        If mBits is negative no zero padding is done.
 *        If mBits is zero the memory for the buffer is allocated dynamically, the
 *        number of bits is not limited.
 *
 * \param pAdts ADTS data handle
 * \param hBs bitstream handle, on which the CRC region referes to
 * \param mBits max number of bits in crc region to be considered
 *
 * \return  ID for the created region, -1 in case of an error
 */
int adtsRead_CrcStartReg(
        HANDLE_ADTS pAdts,
        HANDLE_FDK_BITSTREAM hBs,
        int mBits
        );

/**
 * \brief Ends CRC region identified by reg
 *
 * \param pAdts ADTS data handle
 * \param hBs bitstream handle, on which the CRC region referes to
 * \param reg CRC regions ID returned by adtsRead_CrcStartReg()
 *
 * \return  none
 */
void adtsRead_CrcEndReg(
        HANDLE_ADTS pAdts,
        HANDLE_FDK_BITSTREAM hBs,
        int reg
        );

/**
 * \brief Check CRC
 *
 * Checks if the currently calculated CRC matches the CRC field read from the bitstream
 * Deletes all CRC regions.
 *
 * \param pAdts ADTS data handle
 *
 * \return Returns 0 if they are identical otherwise 1
 */
TRANSPORTDEC_ERROR adtsRead_CrcCheck( HANDLE_ADTS pAdts );


/**
 * \brief Check if we have a valid ADTS frame at the current bitbuffer position
 *
 * This function assumes enough bits in buffer for the current frame.
 * It reads out the header bits to prepare the bitbuffer for the decode loop.
 * In case the header bits show an invalid bitstream/frame, the whole frame is skipped.
 *
 * \param pAdts ADTS data handle which is filled with parsed ADTS header data
 * \param bs handle of bitstream from whom the ADTS header is read
 *
 * \return  error status
 */
TRANSPORTDEC_ERROR adtsRead_DecodeHeader(
        HANDLE_ADTS           pAdts,
        CSAudioSpecificConfig *pAsc,
        HANDLE_FDK_BITSTREAM  bs,
        const INT             ignoreBufferFullness
        );

/**
 * \brief Get the raw data block length of the given block number.
 *
 * \param pAdts ADTS data handle
 * \param blockNum current raw data block index
 * \param pLength pointer to an INT where the length of the given raw data block is stored into
 *                the returned value might be -1, in which case the raw data block length is unknown.
 *
 * \return  error status
 */
int adtsRead_GetRawDataBlockLength(
        HANDLE_ADTS pAdts,
        INT         blockNum
        );


#endif /* TPDEC_ADTS_H */
