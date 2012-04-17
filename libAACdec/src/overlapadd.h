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
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef OVERLAPADD_H
#define OVERLAPADD_H

#include "common_fix.h"

#define  OverlapBufferSize  (1024*2)

typedef FIXP_DBL SPECTRUM[1024];
typedef FIXP_DBL * SPECTRAL_PTR;

#define SPEC_LONG(ptr)       (ptr)
#define SPEC(ptr,w,gl)      ((ptr)+((w)*(gl)))


#endif /* #ifndef OVERLAPADD_H */
