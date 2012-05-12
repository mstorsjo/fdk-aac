/*****************************  MPEG Audio Encoder  ***************************

                     (C) Copyright Fraunhofer IIS (2004-2005)
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
   Initial author:       M. Neuendorf, N. Rettelbach, M. Multrus
   contents/description: PS Parameter extraction, encoding

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
/*!
  \file
  \brief  PS parameter extraction, encoding functions $Revision: 37142 $
*/

#ifndef __INCLUDED_PS_ENCODE_H
#define __INCLUDED_PS_ENCODE_H

#include "ps_const.h"
#include "ps_bitenc.h"


#define IID_SCALE_FT      (64.f)    /* maxVal in Quant tab is +/- 50 */
#define IID_SCALE         6         /* maxVal in Quant tab is +/- 50 */
#define IID_MAXVAL        (1<<IID_SCALE)

#define PS_QUANT_SCALE_FT (64.f)     /* error smaller (64-25)/64 * 20 bands * 4 env -> QuantScale 64 */
#define PS_QUANT_SCALE    6          /* error smaller (64-25)/64 * 20 bands * 4 env -> QuantScale 6 bit */


#define QMF_GROUPS_LO_RES      12
#define SUBQMF_GROUPS_LO_RES   10
#define QMF_GROUPS_HI_RES      18
#define SUBQMF_GROUPS_HI_RES   30


typedef struct T_PS_DATA {

  INT iidEnable;
  INT iidEnableLast;
  INT iidQuantMode;
  INT iidQuantModeLast;
  INT iidDiffMode[PS_MAX_ENVELOPES];
  INT iidIdx     [PS_MAX_ENVELOPES][PS_MAX_BANDS];
  INT iidIdxLast [PS_MAX_BANDS];

  INT iccEnable;
  INT iccEnableLast;
  INT iccQuantMode;
  INT iccQuantModeLast;
  INT iccDiffMode[PS_MAX_ENVELOPES];
  INT iccIdx     [PS_MAX_ENVELOPES][PS_MAX_BANDS];
  INT iccIdxLast [PS_MAX_BANDS];

  INT nEnvelopesLast;

  INT headerCnt;
  INT iidTimeCnt;
  INT iccTimeCnt;
  INT noEnvCnt;

} PS_DATA, *HANDLE_PS_DATA;


typedef struct T_PS_ENCODE{

  PS_DATA         psData;

  PS_BANDS        psEncMode;
  INT             nQmfIidGroups;
  INT             nSubQmfIidGroups;
  INT             iidGroupBorders[QMF_GROUPS_HI_RES + SUBQMF_GROUPS_HI_RES + 1];
  INT             subband2parameterIndex[QMF_GROUPS_HI_RES + SUBQMF_GROUPS_HI_RES];
  UCHAR           iidGroupWidthLd[QMF_GROUPS_HI_RES + SUBQMF_GROUPS_HI_RES];
  FIXP_DBL        iidQuantErrorThreshold;

  UCHAR           psBandNrgScale [PS_MAX_BANDS];

} PS_ENCODE;


typedef struct T_PS_ENCODE *HANDLE_PS_ENCODE;

FDK_PSENC_ERROR FDKsbrEnc_CreatePSEncode(
        HANDLE_PS_ENCODE         *phPsEncode
        );

FDK_PSENC_ERROR FDKsbrEnc_InitPSEncode(
        HANDLE_PS_ENCODE          hPsEncode,
        const PS_BANDS            psEncMode,
        const FIXP_DBL            iidQuantErrorThreshold
        );

FDK_PSENC_ERROR FDKsbrEnc_DestroyPSEncode(
        HANDLE_PS_ENCODE         *phPsEncode
        );

FDK_PSENC_ERROR FDKsbrEnc_PSEncode(
        HANDLE_PS_ENCODE          hPsEncode,
        HANDLE_PS_OUT             hPsOut,
        UCHAR                    *dynBandScale,
        UINT                      maxEnvelopes,
        FIXP_DBL                 *hybridData[HYBRID_FRAMESIZE][MAX_PS_CHANNELS][2],
        const INT                 frameSize,
        const INT                 sendHeader
        );

#endif
