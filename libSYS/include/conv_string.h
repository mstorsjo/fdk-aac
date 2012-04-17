/**************************  Fraunhofer IIS FDK SysLib  **********************

                        (C) Copyright Fraunhofer IIS (2009)
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

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

/** \file   conv_string.h
 *  \brief  String conversion functions.
 */

#ifndef _CONV_STRING_H
#define _CONV_STRING_H



#include "genericStds.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * \brief                Convert a bitbuffer to a hex string.
 * \param string         Hex string.
 * \param charBuf        Input buffer that has to be converted.
 * \param charBufLength  Size of the input buffer.
 * \return               Error code.
 */
INT charBuf2HexString(char *string, UCHAR *charBuf, INT charBufLength);

/**
 * \brief                Convert a hex string to bits.
 * \param string         Hex string that has to be converted.
 * \param charBuf        Output buffer holding the bits, will be filled up with zeros.
 * \param charBufLength  Size of the output buffer.
 * \return               Error code.
 */
INT hexString2CharBuf(const char *string, UCHAR *charBuf, UINT charBufLength);

/**
 * \brief    Convert a character representing a hex sign to the appropriate value.
 * \param c  Character that has to be converted.
 * \return   Converted value (value between 0 and 15), if 16, an error has occured.
 */
UCHAR hexChar2Dec(const char c);

#ifdef __cplusplus
}
#endif


#endif /* _CONV_STRING_H */
