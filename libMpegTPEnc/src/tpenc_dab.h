
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
   contents/description: DAB Transport writer

******************************************************************************/

#ifndef TPENC_DAB_H
#define TPENC_DAB_H



#include "tp_data.h"

#include "FDK_crc.h"

typedef struct {
  USHORT frame_length;
  UCHAR dac_rate;
  UCHAR aac_channel_mode;
  UCHAR sbr_flag;
  UCHAR ps_flag;
  UCHAR mpeg_surround_config;
  UCHAR num_raw_blocks;
  UCHAR BufferFullnesStartFlag;
  int subchannels_num;
  int headerBits;                /*!< Header bit demand for the current raw data block */
  int currentBlock;              /*!< Index of current raw data block */
  int subFrameStartBit;          /*!< Bit position where the current raw data block begins */
  //int subFrameStartPrev;         /*!< Bit position where the previous raw data block begins */
  int crcIndex;
  FDK_CRCINFO crcInfo;
  FDK_CRCINFO crcFire;
  FDK_CRCINFO crcInfo2;
  USHORT tab[256];
} STRUCT_DAB;

typedef STRUCT_DAB *HANDLE_DAB;

/**
 * \brief Initialize DAB data structure
 *
 * \param hDab DAB data handle
 * \param config a valid CODER_CONFIG struct from where the required
 *        information for the DAB header is extrated from
 *
 * \return 0 in case of success.
 */
INT dabWrite_Init(
        HANDLE_DAB   hDab,
        CODER_CONFIG *config
        );

/**
 * \brief Get the total bit overhead caused by DAB
 *
 * \hDab handle to DAB data
 *
 * \return Amount of additional bits required for the current raw data block
 */
int dabWrite_GetHeaderBits( HANDLE_DAB hDab );
int dabWrite_CountTotalBitDemandHeader( HANDLE_DAB hDab, unsigned int streamDataLength );

/**
 * \brief Write an DAB header into the given bitstream. May not write a header
 *        in case of multiple raw data blocks.
 *
 * \param hDab DAB data handle
 * \param hBitStream bitstream handle into which the DAB may be written into
 * \param buffer_fullness the buffer fullness value for the DAB header
 * \param the current raw data block length
 *
 * \return 0 in case of success.
 */
INT dabWrite_EncodeHeader(
        HANDLE_DAB          hDab,
        HANDLE_FDK_BITSTREAM hBitStream,
        int                  bufferFullness,
        int                  frame_length
        );
/**
 * \brief Finish a DAB raw data block
 *
 * \param hDab DAB data handle
 * \param hBs bitstream handle into which the DAB may be written into
 * \param pBits a pointer to a integer holding the current bitstream buffer bit count,
 *              which is corrected to the current raw data block boundary.
 *
 */
void dabWrite_EndRawDataBlock(
        HANDLE_DAB          hDab,
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
 * \param pDab DAB data handle
 * \param hBs bitstream handle of which the CRC region ends
 * \param mBits limit of number of bits to be considered for the requested CRC region
 *
 * \return ID for the created region, -1 in case of an error
 */
int dabWrite_CrcStartReg(
        HANDLE_DAB          pDab,
        HANDLE_FDK_BITSTREAM hBs,
        int                  mBits
        );

/**
 * \brief Ends CRC region identified by reg
 *
 * \param pDab DAB data handle
 * \param hBs bitstream handle of which the CRC region ends
 * \param reg a CRC region ID returned previously by dabWrite_CrcStartReg()
 */
void dabWrite_CrcEndReg(
        HANDLE_DAB          pDab,
        HANDLE_FDK_BITSTREAM hBs,
        int                  reg
        );




#endif /* TPENC_DAB_H */

