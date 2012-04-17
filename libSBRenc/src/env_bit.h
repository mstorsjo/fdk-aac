/****************************************************************************

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


   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

 $Id$

*******************************************************************************/
/*!
  \file
  \brief  Remaining SBR Bit Writing Routines
*/

#ifndef BIT_ENV_H
#define BIT_ENV_H

#include "sbr_encoder.h"
#include "FDK_crc.h"

/* G(x) = x^10 + x^9 + x^5 + x^4 + x + 1 */
#define SBR_CRC_POLY     (0x0233)
#define SBR_CRC_MASK     (0x0200)
#define SBR_CRC_RANGE    (0x03FF)
#define SBR_CRC_MAXREGS     1
#define SBR_CRCINIT      (0x0)


#define SI_SBR_CRC_ENABLE_BITS                  0
#define SI_SBR_CRC_BITS                        10
#define SI_SBR_DRM_CRC_BITS                     8


struct COMMON_DATA;

INT FDKsbrEnc_InitSbrBitstream(struct COMMON_DATA  *hCmonData,
                               UCHAR               *memoryBase,
                               INT                  memorySize,
                               HANDLE_FDK_CRCINFO   hCrcInfo,
                               UINT                 sbrSyntaxFlags);

void
FDKsbrEnc_AssembleSbrBitstream (struct COMMON_DATA  *hCmonData,
                                HANDLE_FDK_CRCINFO   hCrcInfo,
                                INT                  crcReg,
                                UINT                 sbrSyntaxFlags);





#endif /* #ifndef BIT_ENV_H */
