/*****************************  MPEG-4 AAC Decoder  ***************************

                      (C) Copyright Fraunhofer IIS 2000-2008
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
   Author(s):   Robert Weidner (DSP Solutions)
   Description: HCR Decoder: Interface function declaration; common defines
                and structures; defines for switching error-generator,
                -detector, and -concealment

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

*******************************************************************************/

#ifndef _AACDEC_HCR_H_
#define _AACDEC_HCR_H_



#include "channelinfo.h"
#include "FDK_bitstream.h"

void  HcrInitRom (H_HCR_INFO hHcr);
UINT HcrInit(H_HCR_INFO              pHcr,
             CAacDecoderChannelInfo *pAacDecoderChannelInfo,
             const SamplingRateInfo *pSamplingRateInfo,
             HANDLE_FDK_BITSTREAM    bs);
UINT  HcrDecoder (H_HCR_INFO hHcr,
                  CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                  const SamplingRateInfo *pSamplingRateInfo,
                  HANDLE_FDK_BITSTREAM bs);
void  CarryBitToBranchValue(
        UCHAR  carryBit,
        UINT   treeNode,
        UINT  *branchValue,
        UINT  *branchNode
        );

void  CHcr_Read  (HANDLE_FDK_BITSTREAM bs,
                  CAacDecoderChannelInfo *pAacDecoderChannelInfo);
void  HcrMuteErroneousLines(H_HCR_INFO hHcr);

void  setHcrType(H_HCR_INFO hHcr, MP4_ELEMENT_ID type);
INT   getHcrType(H_HCR_INFO hHcr);



#endif /* _AACDEC_HCR_H_ */
