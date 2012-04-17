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
   Description: ADIF reader

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef TPDEC_ADIF_H
#define TPDEC_ADIF_H

#include "tpdec_lib.h"

#define MIN_ADIF_HEADERLENGTH  63  /* in bits */

typedef struct
{
  INT NumProgramConfigElements;
  UINT BitRate;
  UCHAR CopyrightIdPresent;
  UCHAR OriginalCopy;
  UCHAR Home;
  UCHAR BitstreamType;
} CAdifHeader;

/**
 * \brief Parse a ADIF header at the given bitstream and store the parsed data into a given CAdifHeader
 *        and CProgramConfig struct
 *
 * \param pAdifHeader pointer to a CAdifHeader structure to hold the parsed ADIF header data.
 * \param pPce pointer to a CProgramConfig structure where the last PCE will remain.
 *
 * \return TRANSPORTDEC_ERROR error code
 */
TRANSPORTDEC_ERROR adifRead_DecodeHeader(
        CAdifHeader         *pAdifHeader,
        CProgramConfig      *pPce,
        HANDLE_FDK_BITSTREAM bs
        );

#endif /* TPDEC_ADIF_H */
