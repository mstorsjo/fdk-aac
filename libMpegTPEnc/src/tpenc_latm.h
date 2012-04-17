/*****************************  MPEG-4 AAC Encoder  **************************

                        (C) Copyright Fraunhofer IIS (2001)
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
   Author(s):
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef TPENC_LATM_H
#define TPENC_LATM_H



#include "tpenc_lib.h"
#include "FDK_bitstream.h"


#define DEFAULT_LATM_NR_OF_SUBFRAMES 1
#define DEFAULT_LATM_SMC_REPEAT      8

#define MAX_AAC_LAYERS        9

#define LATM_MAX_PROGRAMS     1
#define LATM_MAX_STREAM_ID   16

#define LATM_MAX_LAYERS       1 /*MAX_AAC_LAYERS*/

#define MAX_NR_OF_SUBFRAMES   2           /* set this carefully to avoid buffer overflows */

typedef enum { LATMVAR_SIMPLE_SEQUENCE } LATM_VAR_MODE;

typedef struct {
  signed int frameLengthType;
  signed int frameLengthBits;
  signed int varFrameLengthTable[4];
  signed int streamID;
} LATM_LAYER_INFO;

#define LATM_ASSEMBLEBUF_SIZE ((200+16)*8)
#define LATM_SETUPBUF_SIZE    (200*8)

typedef struct {
  LATM_LAYER_INFO      m_linfo[LATM_MAX_PROGRAMS][LATM_MAX_LAYERS];
  CODER_CONFIG        *config[LATM_MAX_PROGRAMS][LATM_MAX_LAYERS];

  LATM_VAR_MODE        varMode;
  TRANSPORT_TYPE       tt;

  int                  audioMuxLengthBytes;

  int                  audioMuxLengthBytesPos;
  int                  taraBufferFullness; /* state of the bit reservoir */
  int                  varStreamCnt;
  unsigned int         otherDataLenBytes;

  UCHAR                latmFrameCounter;   /* Current frame number. Counts modulo muxConfigPeriod */
  UCHAR                muxConfigPeriod;    /* Distance in frames between MuxConfig */

  UCHAR                audioMuxVersion;    /* AMV1 supports transmission of taraBufferFullness and ASC lengths */
  UCHAR                audioMuxVersionA;   /* for future extensions */

  UCHAR                noProgram;
  UCHAR                noLayer[LATM_MAX_PROGRAMS];
  UCHAR                fractDelayPresent;

  UCHAR                allStreamsSameTimeFraming;
  UCHAR                subFrameCnt;        /* Current Subframe frame */
  UCHAR                noSubframes;        /* Number of subframes    */
  UINT                 latmSubframeStart;  /* Position of current subframe start */
  UCHAR                noSubframes_next;

  UCHAR                fillBits;           /* AudioMuxElement fill bits */
  UCHAR                streamMuxConfigBits;

} LATM_STREAM;

typedef LATM_STREAM *HANDLE_LATM_STREAM;

/**
 * \brief Initialize LATM_STREAM Handle. Creates automatically one program with one layer with
 *        the given layerConfig. The layerConfig must be persisten because references to this pointer
 *        are made at any time again.
 *        Use transportEnc_Latm_AddLayer() to add more programs/layers.
 *
 * \param hLatmStreamInfo HANDLE_LATM_STREAM handle
 * \param hBs Bitstream handle
 * \param layerConfig a valid CODER_CONFIG struct containing the current audio configuration parameters
 * \param audioMuxVersion the LATM audioMuxVersion to be used
 * \param tt the specific TRANSPORT_TYPE to be used, either TT_MP4_LOAS, TT_MP4_LATM_MCP1 or TT_MP4_LATM_MCP0 LOAS
 * \param cb callback information structure.
 *
 * \return an TRANSPORTENC_ERROR error code
 */
TRANSPORTENC_ERROR transportEnc_Latm_Init(
        HANDLE_LATM_STREAM hLatmStreamInfo,
        HANDLE_FDK_BITSTREAM hBs,
        CODER_CONFIG *layerConfig,
        UINT audioMuxVersion,
        TRANSPORT_TYPE tt,
        CSTpCallBacks *cb
        );

/**
 * \brief Get bit demand of next LATM/LOAS header
 *
 * \param hAss HANDLE_LATM_STREAM handle
 * \param streamDataLength the length of the payload
 *
 * \return the number of bits required by the LATM/LOAS headers
 */
unsigned int transportEnc_LatmCountTotalBitDemandHeader (
                                                          HANDLE_LATM_STREAM hAss,
                                                          unsigned int streamDataLength
                                                         );

/**
 * \brief Write LATM/LOAS header into given bitstream handle
 *
 * \param hLatmStreamInfo HANDLE_LATM_STREAM handle
 * \param hBitstream Bitstream handle
 * \param auBits amount of current payload bits
 * \param bufferFullness LATM buffer fullness value
 * \param cb callback information structure.
 *
 * \return an TRANSPORTENC_ERROR error code
 */
TRANSPORTENC_ERROR
transportEnc_LatmWrite (
        HANDLE_LATM_STREAM    hAss,
        HANDLE_FDK_BITSTREAM  hBitstream,
        int                   auBits,
        int                   bufferFullness,
        CSTpCallBacks     *cb
        );

/**
 * \brief Adjust bit count relative to current subframe
 *
 * \param hAss HANDLE_LATM_STREAM handle
 * \param pBits pointer to an int, where the current frame bit count is contained,
 *        and where the subframe relative bit count will be returned into
 *
 * \return void
 */
void transportEnc_LatmAdjustSubframeBits(HANDLE_LATM_STREAM    hAss,
                                         int                  *pBits);

/**
 * \brief Request an LATM frame, which may, or may not be available
 *
 * \param hAss HANDLE_LATM_STREAM handle
 * \param hBs Bitstream handle
 * \param pBytes pointer to an int, where the current frame byte count stored into.
 *        A return value of zero means that currently no LATM/LOAS frame can be returned.
 *        The latter is expected in case of multiple subframes being used.
 *
 * \return void
 */
void transportEnc_LatmGetFrame(
                               HANDLE_LATM_STREAM    hAss,
                               HANDLE_FDK_BITSTREAM  hBs,
                               int                  *pBytes
                               );

/**
 * \brief Write a StreamMuxConfig into the given bitstream handle
 *
 * \param hAss HANDLE_LATM_STREAM handle
 * \param hBs Bitstream handle
 * \param bufferFullness LATM buffer fullness value
 * \param cb callback information structure.
 *
 * \return void
 */
TRANSPORTENC_ERROR
CreateStreamMuxConfig(
        HANDLE_LATM_STREAM hAss,
        HANDLE_FDK_BITSTREAM hBs,
        int bufferFullness,
        CSTpCallBacks *cb
        );


#endif /* TPENC_LATM_H */
