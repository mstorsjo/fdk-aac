/***************************  Fraunhofer IIS FDK Tools  ***********************

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
   Author(s):   Oliver Moser
   Description: bitstream format detection routines

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#if !defined(__BITSTREAM_FORMAT_H__)
#define __BITSTREAM_FORMAT_H__

#include "machine_type.h"
#include "FDK_audio.h"

/**
 * \brief Try to find out the format of a file, given the few first bytes.
 * \param fileData pointer to a buffer holding the first bytes of a file.
 * \param pAu pointer to UCHAR*, returns the address of the first AU found or NULL.
 * \param length pointer to the length of the buffer fileData. Return length of first AU.
 * \return the detected file format, or FF_UNKNOWN in case of failure.
 */
FILE_FORMAT GetFileFormat(UCHAR *fileData, UCHAR **pAu, UINT *length);

/**
 * \brief Try to find out the transport type contained in a given file.
 * \param filename name of the file to be analysed.
 * \param fileFormat pointer to a variable where the detected file format is stored into.
 * \return the detected transport type or TT_UNKNOWN in case of failure.
 */
TRANSPORT_TYPE GetTransportType(const char* filename, FILE_FORMAT *fileFormat);

#endif
