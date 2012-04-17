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
   Author(s):   Manuel Jander
   Description: FDK tools versioning support

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef FDK_CORE_H
#define FDK_CORE_H

#include "FDK_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get FDK_tools library information.
 *  @return     Return 0 on success and a negative errorcode on failure (see errorcodes.h).
 */
int FDK_toolsGetLibInfo(LIB_INFO *info);

#ifdef __cplusplus
}
#endif

#endif
