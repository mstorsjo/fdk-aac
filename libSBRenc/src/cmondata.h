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

****************************************************************************/
/*!
  \file
  \brief  Core Coder's and SBR's shared data structure definition $Revision: 36847 $
*/
#ifndef __SBR_CMONDATA_H
#define __SBR_CMONDATA_H

#include "FDK_bitstream.h"


struct COMMON_DATA {
  INT                   sbrHdrBits;             /**< number of SBR header bits */
  INT                   sbrDataBits;            /**< number of SBR data bits */
  INT                   sbrFillBits;            /**< number of SBR fill bits */
  FDK_BITSTREAM         sbrBitbuf;              /**< the SBR data bitbuffer */
  FDK_BITSTREAM         tmpWriteBitbuf;         /**< helper var for writing header*/
  INT                   xOverFreq;              /**< the SBR crossover frequency */
  INT                   dynBwEnabled;           /**< indicates if dynamic bandwidth is enabled */
  INT                   sbrNumChannels;         /**< number of channels (meaning mono or stereo) */
  INT                   dynXOverFreqEnc;        /**< encoder dynamic crossover frequency */
};

typedef struct COMMON_DATA *HANDLE_COMMON_DATA;



#endif
