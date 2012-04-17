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

/*!< If MPFWRITE_MP4FF_ENABLE is set, include support for MPEG ISO fileformat.
     If not set, no .mp4, .m4a and .3gp files can be used for input.  */
/* #define MPFWRITE_MP4FF_ENABLE */

typedef struct STRUCT_FILEWRITE *HANDLE_FILEWRITE;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief                         Open an MPEG audio file.
 * \param mpegFileWrite_Filename  String of the filename to be opened.
 * \param fileFmt                 Transport format to use.
 * \param conf
 * \param confSize
 * \return                        MPEG file write handle.
 */
HANDLE_FILEWRITE mpegFileWrite_Open( char           *mpegFileWrite_Filename,
                                     FILE_FORMAT     fileFmt,
                                     TRANSPORT_TYPE  transportType,
                                     UCHAR          *conf,
                                     UINT            confSize
                                   );

/**
 * \brief             Write to an MPEG audio file.
 * \param inBuffer    Buffer to write.
 * \param bufferSize  Size of buffer to write in bytes.
 * \return            0 on sucess, -1 on unsupported file format or write error.
 */
int mpegFileWrite_Write( HANDLE_FILEWRITE  hFileWrite,
                         UCHAR            *inBuffer,
                         int              bufferSize
                       );

/**
 * \brief             Deallocate memory and close file.
 * \param hFileWrite  MPEG file write handle.
 * \return            0 on sucess.
 */
int mpegFileWrite_Close( HANDLE_FILEWRITE *hFileWrite );


#ifdef __cplusplus
}
#endif
