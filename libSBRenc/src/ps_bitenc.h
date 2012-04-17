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
   Initial author:       N. Rettelbach
   contents/description: Parametric Stereo bitstream encoder

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "ps_main.h"
#include "ps_const.h"
#include "FDK_bitstream.h"

#ifndef PS_BITENC_H
#define PS_BITENC_H

typedef struct T_PS_OUT {

  INT         enablePSHeader;
  INT         enableIID;
  INT         iidMode;
  INT         enableICC;
  INT         iccMode;
  INT         enableIpdOpd;

  INT         frameClass;
  INT         nEnvelopes;
  /* ENV data */
  INT         frameBorder[PS_MAX_ENVELOPES];

  /* iid data  */
  PS_DELTA    deltaIID[PS_MAX_ENVELOPES];
  INT         iid[PS_MAX_ENVELOPES][PS_MAX_BANDS];
  INT         iidLast[PS_MAX_BANDS];

  /* icc data  */
  PS_DELTA    deltaICC[PS_MAX_ENVELOPES];
  INT         icc[PS_MAX_ENVELOPES][PS_MAX_BANDS];
  INT         iccLast[PS_MAX_BANDS];

  /* ipd data  */
  PS_DELTA    deltaIPD[PS_MAX_ENVELOPES];
  INT         ipd[PS_MAX_ENVELOPES][PS_MAX_BANDS];
  INT         ipdLast[PS_MAX_BANDS];

  /* opd data  */
  PS_DELTA    deltaOPD[PS_MAX_ENVELOPES];
  INT         opd[PS_MAX_ENVELOPES][PS_MAX_BANDS];
  INT         opdLast[PS_MAX_BANDS];

} PS_OUT, *HANDLE_PS_OUT;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INT FDKsbrEnc_EncodeIid(HANDLE_FDK_BITSTREAM  hBitBuf,
                        const INT      *iidVal,
                        const INT      *iidValLast,
                        const INT       nBands,
                        const PS_IID_RESOLUTION res,
                        const PS_DELTA  mode,
                        INT            *error);

INT FDKsbrEnc_EncodeIcc(HANDLE_FDK_BITSTREAM  hBitBuf,
                        const INT      *iccVal,
                        const INT      *iccValLast,
                        const INT       nBands,
                        const PS_DELTA  mode,
                        INT            *error);

INT FDKsbrEnc_EncodeIpd(HANDLE_FDK_BITSTREAM  hBitBuf,
                        const INT      *ipdVal,
                        const INT      *ipdValLast,
                        const INT       nBands,
                        const PS_DELTA  mode,
                        INT            *error);

INT FDKsbrEnc_EncodeOpd(HANDLE_FDK_BITSTREAM  hBitBuf,
                        const INT      *opdVal,
                        const INT      *opdValLast,
                        const INT       nBands,
                        const PS_DELTA  mode,
                        INT            *error);

INT FDKsbrEnc_WritePSBitstream(const HANDLE_PS_OUT   psOut,
                               HANDLE_FDK_BITSTREAM  hBitBuf);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* #ifndef PS_BITENC_H */
