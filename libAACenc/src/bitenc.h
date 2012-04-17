/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (1999)
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
   Initial author:       M. Werner
   contents/description: Bitstream encoder

******************************************************************************/
#ifndef _BITENC_H
#define _BITENC_H


#include "qc_data.h"
#include "aacenc_tns.h"
#include "channel_map.h"
#include "interface.h"    /* obsolete, when PSY_OUT is thrown out of the WritBS-call! */
#include "FDK_audio.h"
#include "aacenc.h"

#include "tpenc_lib.h"

typedef enum{
  MAX_ENCODER_CHANNELS  = 9,
  MAX_BLOCK_TYPES       = 4,
  MAX_AAC_LAYERS        = 9,
  MAX_LAYERS            = MAX_AAC_LAYERS  , /* only one core layer if present */
  FIRST_LAY             = 1   /* default layer number for AAC nonscalable */
} _MAX_CONST;

#define BUFFER_MX_HUFFCB_SIZE (32*sizeof(INT))  /* our FDK_bitbuffer needs size of power 2 */

#define EL_ID_BITS  ( 3 )


/**
 * \brief Arbitrary order bitstream writer. This function can either assemble a bit stream
 *        and write into the bit buffer of hTpEnc or calculate the number of static bits (signal independent)
 *        TpEnc handle must be NULL in this case. Or also Calculate the minimum possible number of
 *        static bits which by disabling all tools e.g. MS, TNS and sbfCnt=0. The minCnt parameter
 *        has to be 1 in this latter case.
 * \param hTpEnc Transport encoder handle. If NULL, the number of static bits will be returned into
 *        *pBitDemand.
 * \param pElInfo
 * \param qcOutChannel
 * \param hReorderInfo
 * \param psyOutElement
 * \param psyOutChannel
 * \param syntaxFlags Bit stream syntax flags as defined in FDK_audio.h (Audio Codec flags).
 * \param aot
 * \param epConfig
 * \param pBitDemand Pointer to an int where the amount of bits is returned into. The returned value
 *        depends on if hTpEnc is NULL and minCnt.
 * \param minCnt If non-zero the value returned into *pBitDemand is the absolute minimum required amount of
 *        static bits in order to write a valid bit stream.
 * \return AAC_ENCODER_ERROR error code
 */
AAC_ENCODER_ERROR FDKaacEnc_ChannelElementWrite( HANDLE_TRANSPORTENC  hTpEnc,
                                                 ELEMENT_INFO        *pElInfo,
                                                 QC_OUT_CHANNEL      *qcOutChannel[(2)],
                                                 PSY_OUT_ELEMENT     *psyOutElement,
                                                 PSY_OUT_CHANNEL     *psyOutChannel[(2)],
                                                 UINT                 syntaxFlags,
                                                 AUDIO_OBJECT_TYPE    aot,
                                                 SCHAR                epConfig,
                                                 INT                 *pBitDemand,
                                                 UCHAR                minCnt
                                               );
/**
 * \brief Write bit stream or account static bits
 * \param hTpEnc transport encoder handle. If NULL, the function will
 *        not write any bit stream data but only count the amount
 *        of static (signal independent) bits
 * \param channelMapping Channel mapping info
 * \param qcOut
 * \param psyOut
 * \param qcKernel
 * \param hBSE
 * \param aot Audio Object Type being encoded
 * \param syntaxFlags Flags indicating format specific detail
 * \param epConfig Error protection config
 */
AAC_ENCODER_ERROR FDKaacEnc_WriteBitstream (HANDLE_TRANSPORTENC hTpEnc,
                                            CHANNEL_MAPPING *channelMapping,
                                            QC_OUT* qcOut,
                                            PSY_OUT* psyOut,
                                            QC_STATE* qcKernel,
                                            AUDIO_OBJECT_TYPE  aot,
                                            UINT  syntaxFlags,
                                            SCHAR  epConfig
                                           );

INT FDKaacEnc_writeExtensionData( HANDLE_TRANSPORTENC  hTpEnc,
                                  QC_OUT_EXTENSION    *pExtension,
                                  INT                  elInstanceTag,
                                  UINT                 alignAnchor,
                                  UINT                 syntaxFlags,
                                  AUDIO_OBJECT_TYPE    aot,
                                  SCHAR                epConfig
                                );

#endif /* _BITENC_H */
