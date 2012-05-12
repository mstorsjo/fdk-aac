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
   Description: temporal noise shaping tool

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef TNS_H
#define TNS_H

#include "common_fix.h"

enum
{
  TNS_MAX_WINDOWS = 8,    /* 8 */
  TNS_MAXIMUM_ORDER   = 20,  /* 12 for AAC-LC and AAC-SSR. Set to 20 for AAC-Main (AOT 1). Some broken encoders also do order 20 for AAC-LC :( */
  TNS_MAXIMUM_FILTERS = 3
};

typedef struct
{
  SCHAR Coeff[TNS_MAXIMUM_ORDER];

  UCHAR StartBand;
  UCHAR StopBand;

  SCHAR Direction;
  SCHAR Resolution;

  UCHAR Order;
} CFilter;

typedef struct
{
  CFilter Filter[TNS_MAX_WINDOWS][TNS_MAXIMUM_FILTERS];
  UCHAR NumberOfFilters[TNS_MAX_WINDOWS];
  UCHAR DataPresent;
  UCHAR Active;
} CTnsData;

void CTns_Reset(CTnsData *pTnsData);

#endif /* #ifndef TNS_H */
