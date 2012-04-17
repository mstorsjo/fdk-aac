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

#include "FDK_core.h"

/* FDK tools library info */
#define FDK_TOOLS_LIB_VL0 2
#define FDK_TOOLS_LIB_VL1 2
#define FDK_TOOLS_LIB_VL2 5
#define FDK_TOOLS_LIB_TITLE "FDK Tools"
#define FDK_TOOLS_LIB_BUILD_DATE __DATE__
#define FDK_TOOLS_LIB_BUILD_TIME __TIME__

int FDK_toolsGetLibInfo(LIB_INFO *info)
{
  UINT v;
  int i;

  if (info == NULL) {
    return -1;
  }

  /* search for next free tab */
  i = FDKlibInfo_lookup(info, FDK_TOOLS);
  if (i<0) return -1;

  info += i;

  v = LIB_VERSION(FDK_TOOLS_LIB_VL0, FDK_TOOLS_LIB_VL1, FDK_TOOLS_LIB_VL2);

  FDKsprintf(info->versionStr, "%d.%d.%d", ((v >> 24) & 0xff), ((v >> 16) & 0xff), ((v >> 8 ) & 0xff));

  info->module_id = FDK_TOOLS;
  info->version = v;
  info->build_date = (char *)FDK_TOOLS_LIB_BUILD_DATE;
  info->build_time = (char *)FDK_TOOLS_LIB_BUILD_TIME;
  info->title      = (char *)FDK_TOOLS_LIB_TITLE;
  info->flags = 0;

  return 0;
}
