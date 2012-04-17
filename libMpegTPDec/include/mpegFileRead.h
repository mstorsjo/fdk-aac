/*****************************  MPEG-4 AAC Decoder  **************************

                        (C) Copyright Fraunhofer IIS (2006)
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
   Author(s):   Manuel Jander
   Description: Bitstream data provider for MP4 decoders

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "machine_type.h"
#include "FDK_audio.h"

#define MPFREAD_MP4FF_DISABLE

#ifndef MPFREAD_MP4FF_DISABLE
  /*!< If MPFREAD_MP4FF_ENABLE is set, include support for MPEG ISO fileformat.
       If not set, no .mp4, .m4a and .3gp files can be used for input. */
  #define MPFREAD_MP4FF_ENABLE
#endif

/* maximum number of layers which can be read        */
/* shall equal max number of layers read by iisisoff */
#define FILEREAD_MAX_LAYERS (2)

typedef struct STRUCT_FILEREAD *HANDLE_FILEREAD;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief           Open an MPEG audio file and try to detect its format.
 * \param filename  String of the filename to be opened.
 * \param fileFormat Skip file format detection and use given format if fileFormat != FF_UNKNOWN.
                    Else store detected format into *fileFmt.
 * \param transportType Skip transport type detection and use given format if transportType != TT_UNKNOWN.
                    Else store detected format into *fileFmt.
 * \param conf      Pointer to unsigned char to hold the AudioSpecificConfig of the input file, if
                    any (MPEG 4 file format). In case of RAW LATM it holds the StreamMuxConfig.
 * \param confSize  Pointer to an integer, where the length of the ASC or SMC (in case of RAW LATM)
                    is stored to.
 * \return          MPEG file read handle.
 */
HANDLE_FILEREAD mpegFileRead_Open( const char     *filename,
                                   FILE_FORMAT     fileFormat,
                                   TRANSPORT_TYPE  transportType,
                                   UCHAR          *conf[],
                                   UINT            confSize[],
                                   INT            *noOfLayers
                                 );

/**
 * \brief           Get the file format of the input file.
 * \param hDataSrc  MPEG file read handle.
 * \return          File format of the input file.
 */
FILE_FORMAT mpegFileRead_GetFileFormat(HANDLE_FILEREAD hDataSrc);

/**
 * \brief           Get the transport type of the input file.
 * \param hDataSrc  MPEG file read handle.
 * \return          Transport type of the input file.
 */
TRANSPORT_TYPE mpegFileRead_GetTransportType(HANDLE_FILEREAD hDataSrc);

/**
 * \brief Read data from MPEG file. In case of packet file, read one packet, in case
 *        of streaming file with embedded synchronisation layer (LOAS/ADTS...), just
 *        fill the buffer.
 *
 * \param hMpegFile   MPEG file read handle.
 * \param inBuffer    Pointer to input buffer.
 * \param bufferSize  Size of input buffer.
 * \param bytesValid  Number of bytes that were read.
 * \return            0 on success, -1 if unsupported file format or file read error.
 */
int mpegFileRead_Read( HANDLE_FILEREAD   hMpegFile,
                       UCHAR            *inBuffer[],
                       UINT              bufferSize,
                       UINT             *bytesValid
                     );

/**
 * \brief            Seek in file from origin by given offset in frames.
 * \param hMpegFile  MPEG file read handle.
 * \param origin     If 0, the origin is the file beginning (absolute seek).
 *                   If 1, the origin is the current position (relative seek).
 * \param offset     The amount of frames to seek from the given origin.
 * \return           0 on sucess, -1 if offset < 0 or file read error.
 */
int mpegFileRead_seek( HANDLE_FILEREAD   hMpegFile,
                       INT               origin,
                       INT               offset
                      );

/**
 * \brief            Get file position in percent.
 * \param hMpegFile  MPEG file read handle.
 * \return           File position in percent.
 */
int mpegFileRead_getPercent(HANDLE_FILEREAD hMpegFile);


/**
 * \brief           Close MPEG audio file.
 * \param hMpegFile Mpeg file read handle.
 * \return          0 on sucess.
 */
int mpegFileRead_Close(HANDLE_FILEREAD *hMpegFile);

#ifdef __cplusplus
}
#endif
