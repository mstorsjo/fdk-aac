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
   Description: long/short-block decoding

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef BLOCK_H
#define BLOCK_H

#include "common_fix.h"

#include "channelinfo.h"
#include "FDK_bitstream.h"

/* PNS (of block) */
void CPns_Read (CPnsData *pPnsData,
                HANDLE_FDK_BITSTREAM bs,
                const CodeBookDescription *hcb,
                SHORT *pScaleFactor,
                UCHAR global_gain,
                int band,
                int group);


void CPns_Apply (const CPnsData *pPnsData,
                 const CIcsInfo *pIcsInfo,
                 SPECTRAL_PTR pSpectrum,
                 const SHORT    *pSpecScale,
                 const SHORT    *pScaleFactor,
                 const SamplingRateInfo *pSamplingRateInfo,
                 const INT granuleLength,
                 const int channel);



/* TNS (of block) */
/*!
  \brief Read tns data-present flag from bitstream

  The function reads the data-present flag for tns from
  the bitstream.

  \return  none
*/
void CTns_ReadDataPresentFlag(HANDLE_FDK_BITSTREAM bs,
                              CTnsData *pTnsData);

void CTns_ReadDataPresentUsac(
        HANDLE_FDK_BITSTREAM hBs,
        CTnsData *pTnsData0,
        CTnsData *pTnsData1,
        const CIcsInfo *pIcsInfo,
        const UINT flags,
        const int fCommonWindow
        );

AAC_DECODER_ERROR CTns_Read(HANDLE_FDK_BITSTREAM bs,
                            CTnsData *pTnsData,
                            const CIcsInfo *pIcsInfo,
                            const UINT flags);

void CTns_Apply ( CTnsData *RESTRICT pTnsData, /*!< pointer to aac decoder info */
                  const CIcsInfo *pIcsInfo,
                  SPECTRAL_PTR pSpectralCoefficient,
                  const SamplingRateInfo *pSamplingRateInfo,
                  const INT granuleLength);

/* Block */

LONG CBlock_GetEscape(HANDLE_FDK_BITSTREAM bs, const LONG q);

/**
 * \brief Read scale factor data. See chapter 4.6.2.3.2 of ISO/IEC 14496-3.
 *        The SF_OFFSET = 100 value referenced in chapter 4.6.2.3.3 is already substracted
 *        from the scale factor values. Also includes PNS data reading.
 * \param bs bit stream handle data source
 * \param pAacDecoderChannelInfo channel context info were decoded data is stored into.
 * \param flags the decoder flags.
 */
AAC_DECODER_ERROR CBlock_ReadScaleFactorData(
        CAacDecoderChannelInfo *pAacDecoderChannelInfo,
        HANDLE_FDK_BITSTREAM bs,
        const UINT flags
        );

/**
 * \brief Read Huffman encoded spectral data.
 * \param pAacDecoderChannelInfo channel context info.
 * \param pSamplingRateInfo sampling rate info (sfb offsets).
 * \param flags syntax flags.
 */
AAC_DECODER_ERROR  CBlock_ReadSpectralData(HANDLE_FDK_BITSTREAM bs,
                                           CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                                           const SamplingRateInfo *pSamplingRateInfo,
                                           const UINT  flags);


AAC_DECODER_ERROR CBlock_ReadSectionData(HANDLE_FDK_BITSTREAM bs,
                                         CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                                         const SamplingRateInfo *pSamplingRateInfo,
                                         const UINT  flags);

/**
 * \brief find a common exponent (shift factor) for all sfb in each Spectral window, and store them into
 *        CAacDecoderChannelInfo::specScale.
 * \param pAacDecoderChannelInfo channel context info.
 * \param pSamplingRateInfo sampling rate info (sfb offsets).
 */
void CBlock_ScaleSpectralData(CAacDecoderChannelInfo *pAacDecoderChannelInfo, SamplingRateInfo *pSamplingRateInfo);

/**
 * \brief Apply TNS and PNS tools.
 */
void ApplyTools ( CAacDecoderChannelInfo *pAacDecoderChannelInfo[],
                  const SamplingRateInfo *pSamplingRateInfo,
                  const UINT flags,
                  const int channel );

/**
 * \brief Transform MDCT spectral data into time domain
 */
void CBlock_FrequencyToTime(CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo,
                            CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                            INT_PCM outSamples[],
                            const SHORT frameLen,
                            const int stride,
                            const int frameOk,
                            FIXP_DBL *pWorkBuffer1);

/**
 * \brief Transform double lapped MDCT (AAC-ELD) spectral data into time domain.
 */
void CBlock_FrequencyToTimeLowDelay(CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo,
                                    CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                                    INT_PCM outSamples[],
                                    const short frameLen,
                                    const char stride);

AAC_DECODER_ERROR CBlock_InverseQuantizeSpectralData(CAacDecoderChannelInfo *pAacDecoderChannelInfo, SamplingRateInfo *pSamplingRateInfo);

/**
 * \brief Calculate 2^(lsb/4) * value^(4/3)
 * \param pValue pointer to quantized value. The inverse quantized result is stored back here.
 * \param lsb 2 LSBs of the scale factor (scaleFactor % 4) applied as power 2 factor to the
 *        resulting inverse quantized value.
 * \return the exponent of the result (mantissa) stored into *pValue.
 */
FDK_INLINE
int EvaluatePower43 ( FIXP_DBL *pValue,
                      UINT lsb )
{
  FIXP_DBL value;
  UINT freeBits;
  UINT exponent;

  value = *pValue;
  freeBits = fNormz (value) ;
  exponent = DFRACT_BITS - freeBits ;
  FDK_ASSERT (exponent < 14);

  UINT x = (((int)value << freeBits) >> 19) ;
  UINT tableIndex = (x & 0x0FFF) >> 4 ;
  FIXP_DBL invQVal ;

  x = x & 0x0F;

  UINT r0=(LONG)InverseQuantTable [tableIndex+0];
  UINT r1=(LONG)InverseQuantTable [tableIndex+1];
  USHORT nx=16-x;
  UINT temp=(r0)*nx+(r1)*x;
  invQVal = (FIXP_DBL)temp;

  FDK_ASSERT(lsb < 4);
  *pValue = fMultDiv2 (invQVal, MantissaTable [lsb][exponent]) ;

  /* + 1 compensates fMultDiv2(). */
  return ExponentTable [lsb][exponent] + 1;
}

/**
 * \brief determine the required shift scale for the given quantized value and scale (factor % 4) value.
 */
FDK_INLINE int GetScaleFromValue (FIXP_DBL value, unsigned int lsb)
{
  if (value!=(FIXP_DBL)0)
  {
    int scale = EvaluatePower43 (&value, lsb) ;
    return CntLeadingZeros (value) - scale - 2 ;
  }
  else
    return 0; /* Return zero, because its useless to scale a zero value, saves workload and avoids scaling overshifts. */
}


//#ifdef AACDEC_HUFFMANDECODER_ENABLE

/*!
  \brief Read huffman codeword

  The function reads the huffman codeword from the bitstream and
  returns the index value.

  \return  index value
*/
inline int CBlock_DecodeHuffmanWord( HANDLE_FDK_BITSTREAM bs,                           /*!< pointer to bitstream */
                                     const CodeBookDescription *hcb )                   /*!< pointer to codebook description */
{
  UINT val;
  UINT index = 0;
  const USHORT (*CodeBook) [HuffmanEntries] = hcb->CodeBook;

  while (1)
  {
    val = CodeBook[index][FDKreadBits(bs,HuffmanBits)]; /* Expensive memory access */

    if ((val & 1) == 0)
    {
      index = val>>2;
      continue;
    }
    else
    {
      if (val & 2)
      {
        FDKpushBackCache(bs,1);
      }

      val >>= 2;
      break;
    }
  }

  return val;
}

//#endif /* AACDEC_HUFFMANDECODER_ENABLE */

#endif /* #ifndef BLOCK_H */
