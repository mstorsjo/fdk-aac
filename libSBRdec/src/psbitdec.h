/****************************************************************************

                     (C) Copyright Fraunhofer IIS (2005)
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

#ifndef __PSBITDEC_H
#define __PSBITDEC_H

#include "sbrdecoder.h"


#include "psdec.h"


unsigned int
ReadPsData (struct PS_DEC  *h_ps_d,
            HANDLE_FDK_BITSTREAM  hBs,
            int nBitsLeft);

int
DecodePs(struct PS_DEC *h_ps_d,
         const UCHAR    frameError);


#endif /* __PSBITDEC_H */
