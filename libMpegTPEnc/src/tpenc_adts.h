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
   contents/description: ADTS Transport writer

******************************************************************************/

#ifndef TPENC_ADTS_H
#define TPENC_ADTS_H



#include "tp_data.h"

#include "FDK_crc.h"

typedef struct {
  INT sample_freq;
  CHANNEL_MODE channel_mode;
  UCHAR decoderCanDoMpeg4;
  UCHAR mpeg_id;
  UCHAR layer;
  UCHAR protection_absent;
  UCHAR profile;
  UCHAR sample_freq_index;
  UCHAR private_bit;
  UCHAR original;
  UCHAR home;
  UCHAR copyright_id;
  UCHAR copyright_start;
  USHORT frame_length;
  UCHAR num_raw_blocks;
  UCHAR BufferFullnesStartFlag;
  int headerBits;                /*!< Header bit demand for the current raw data block */
  int currentBlock;              /*!< Index of current raw data block */
  int subFrameStartBit;          /*!< Bit position where the current raw data block begins */
  FDK_CRCINFO crcInfo;
} STRUCT_ADTS;

typedef STRUCT_ADTS *HANDLE_ADTS;

/**
 * \brief Initialize ADTS data structure
 *
 * \param hAdts ADTS data handle
 * \param config a valid CODER_CONFIG struct from where the required
 *        information for the ADTS header is extrated from
 *
 * \return 0 in case of success.
 */
INT adtsWrite_Init(
        HANDLE_ADTS   hAdts,
        CODER_CONFIG *config
        );

/**
 * \brief Get the total bit overhead caused by ADTS
 *
 * \hAdts handle to ADTS data
 *
 * \return Amount of additional bits required for the current raw data block
 */
int adtsWrite_GetHeaderBits( HANDLE_ADTS hAdts );

/**
 * \brief Write an ADTS header into the given bitstream. May not write a header
 *        in case of multiple raw data blocks.
 *
 * \param hAdts ADTS data handle
 * \param hBitStream bitstream handle into which the ADTS may be written into
 * \param buffer_fullness the buffer fullness value for the ADTS header
 * \param the current raw data block length
 *
 * \return 0 in case of success.
 */
INT adtsWrite_EncodeHeader(
        HANDLE_ADTS          hAdts,
        HANDLE_FDK_BITSTREAM hBitStream,
        int                  bufferFullness,
        int                  frame_length
        );
/**
 * \brief Finish a ADTS raw data block
 *
 * \param hAdts ADTS data handle
 * \param hBs bitstream handle into which the ADTS may be written into
 * \param pBits a pointer to a integer holding the current bitstream buffer bit count,
 *              which is corrected to the current raw data block boundary.
 *
 */
void adtsWrite_EndRawDataBlock(
        HANDLE_ADTS          hAdts,
        HANDLE_FDK_BITSTREAM hBs,
        int                 *bits
        );


/**
 * \brief Start CRC region with a maximum number of bits
 *        If mBits is positive zero padding will be used for CRC calculation, if there
 *        are less than mBits bits available.
 *        If mBits is negative no zero padding is done.
 *        If mBits is zero the memory for the buffer is allocated dynamically, the
 *        number of bits is not limited.
 *
 * \param pAdts ADTS data handle
 * \param hBs bitstream handle of which the CRC region ends
 * \param mBits limit of number of bits to be considered for the requested CRC region
 *
 * \return ID for the created region, -1 in case of an error
 */
int adtsWrite_CrcStartReg(
        HANDLE_ADTS          pAdts,
        HANDLE_FDK_BITSTREAM hBs,
        int                  mBits
        );

/**
 * \brief Ends CRC region identified by reg
 *
 * \param pAdts ADTS data handle
 * \param hBs bitstream handle of which the CRC region ends
 * \param reg a CRC region ID returned previously by adtsWrite_CrcStartReg()
 */
void adtsWrite_CrcEndReg(
        HANDLE_ADTS          pAdts,
        HANDLE_FDK_BITSTREAM hBs,
        int                  reg
        );




#endif /* TPENC_ADTS_H */

