/*****************************  MPEG-4 AAC Decoder  **************************

                        (C) Copyright Fraunhofer IIS (2010)
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
   Author(s):   Christian Griebel
   Description: Dynamic range control (DRC) global data types

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef AACDEC_DRC_TYPES_H
#define AACDEC_DRC_TYPES_H



#include "common_fix.h"

 #define MAX_DRC_THREADS      (   3 )        /* Heavy compression value is handled just like MPEG DRC data */
#define MAX_DRC_BANDS         (  16 )        /* 2^LEN_DRC_BAND_INCR (LEN_DRC_BAND_INCR = 4) */

/**
 * \brief DRC module global data types
 */
typedef enum
{
  UNKNOWN_PAYLOAD   = 0,
  MPEG_DRC_EXT_DATA,
  DVB_DRC_ANC_DATA

} AACDEC_DRC_PAYLOAD_TYPE;

typedef struct
{
  UINT    expiryCount;
  UINT    numBands;
  USHORT  bandTop[MAX_DRC_BANDS];
  SHORT   drcInterpolationScheme;
  UCHAR   drcValue[MAX_DRC_BANDS];

} CDrcChannelData;

typedef struct
{
  AACDEC_DRC_PAYLOAD_TYPE  type;
  UINT   excludedChnsMask;
  SCHAR  progRefLevel;
  SCHAR  pceInstanceTag;

  CDrcChannelData channelData;

} CDrcPayload;

typedef struct
{
  FIXP_DBL  cut;
  FIXP_DBL  boost;

  UINT   expiryFrame;
  SCHAR  targetRefLevel;
  UCHAR  bsDelayEnable;
  UCHAR  applyHeavyCompression;

} CDrcParams;


typedef struct
{
  CDrcParams params;                         /* Module parameters that can be set by user (via SetParam API function) */

  UCHAR  enable;                             /* Switch that controls dynamic range processing */
  UCHAR  digitalNorm;                        /* Switch to en-/disable reference level normalization in digital domain */

  USHORT numPayloads;                        /* The number of DRC data payload elements found within frame */
  USHORT numThreads;                         /* The number of DRC data threads extracted from the found payload elements */
  SCHAR  progRefLevel;                       /* Program reference level for all channels */

  UCHAR  dvbAncDataAvailable;                  /* Flag that indicates whether DVB ancillary data is present or not */
  UINT   dvbAncDataPosition;                   /* Used to store the DVB ancillary data payload position in the bitstream (only one per frame) */
  UINT   drcPayloadPosition[MAX_DRC_THREADS];  /* Used to store the DRC payload positions in the bitstream */

} CDrcInfo;

typedef CDrcInfo *HANDLE_AAC_DRC;

#endif /* AACDEC_DRC_TYPES_H */

