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

#include "aacdec_tns.h"
#include "aac_rom.h"
#include "FDK_bitstream.h"
#include "channelinfo.h"



/*!
  \brief Reset tns data

  The function resets the tns data

  \return  none
*/
void CTns_Reset(CTnsData *pTnsData)
{
  /* Note: the following FDKmemclear should not be required. */
  FDKmemclear(pTnsData->Filter, TNS_MAX_WINDOWS*TNS_MAXIMUM_FILTERS*sizeof(CFilter));
  FDKmemclear(pTnsData->NumberOfFilters, TNS_MAX_WINDOWS*sizeof(UCHAR));
  pTnsData->DataPresent = 0;
  pTnsData->Active = 0;
}

void CTns_ReadDataPresentFlag(HANDLE_FDK_BITSTREAM bs,    /*!< pointer to bitstream */
                              CTnsData *pTnsData)         /*!< pointer to aac decoder channel info */
{
  pTnsData->DataPresent = (UCHAR) FDKreadBits(bs,1);
}

/*!
  \brief Read tns data from bitstream

  The function reads the elements for tns from
  the bitstream.

  \return  none
*/
AAC_DECODER_ERROR CTns_Read(HANDLE_FDK_BITSTREAM bs,
                            CTnsData *pTnsData,
                            const CIcsInfo *pIcsInfo,
                            const UINT flags)
{
  UCHAR n_filt,order;
  UCHAR length,coef_res,coef_compress;
  UCHAR window;
  UCHAR wins_per_frame = GetWindowsPerFrame(pIcsInfo);
  UCHAR isLongFlag = IsLongBlock(pIcsInfo);
  AAC_DECODER_ERROR ErrorStatus = AAC_DEC_OK;

  if (!pTnsData->DataPresent) {
    return ErrorStatus;
  }

  for (window = 0; window < wins_per_frame; window++)
  {
    pTnsData->NumberOfFilters[window] = n_filt = (UCHAR) FDKreadBits(bs, isLongFlag ? 2 : 1);

    if (pTnsData->NumberOfFilters[window] > TNS_MAXIMUM_FILTERS){
        pTnsData->NumberOfFilters[window] = n_filt = TNS_MAXIMUM_FILTERS;
    }

    if (n_filt)
    {
      int index;
      UCHAR nextstopband;

      coef_res = (UCHAR) FDKreadBits(bs,1);

      nextstopband = GetScaleFactorBandsTotal(pIcsInfo);

      for (index=0; index < n_filt; index++)
      {
        CFilter *filter = &pTnsData->Filter[window][index];

        length = (UCHAR)FDKreadBits(bs, isLongFlag ? 6 : 4);

        if (length > nextstopband){
          length = nextstopband;
        }

        filter->StartBand = nextstopband - length;
        filter->StopBand  = nextstopband;
        nextstopband = filter->StartBand;

        {
          filter->Order = order = (UCHAR) FDKreadBits(bs, isLongFlag ? 5 : 3);
        }

        if (filter->Order > TNS_MAXIMUM_ORDER){
          filter->Order = order = TNS_MAXIMUM_ORDER;
        }

        if (order)
        {
          UCHAR coef,s_mask;
          UCHAR i;
          SCHAR n_mask;
          static const UCHAR sgn_mask[] = {  0x2,  0x4,  0x8 };
          static const SCHAR neg_mask[] = { ~0x3, ~0x7, ~0xF };

          filter->Direction = FDKreadBits(bs,1) ? -1 : 1;

          coef_compress = (UCHAR) FDKreadBits(bs,1);

          filter->Resolution = coef_res + 3;

          s_mask = sgn_mask[coef_res + 1 - coef_compress];
          n_mask = neg_mask[coef_res + 1 - coef_compress];

          for (i=0; i < order; i++)
          {
            coef = (UCHAR) FDKreadBits(bs,filter->Resolution - coef_compress);
            filter->Coeff[i] = (coef & s_mask) ? (coef | n_mask) : coef;
          }
        }
      }
    }
  }

  pTnsData->Active = 1;

  return ErrorStatus;
}


static void CTns_Filter (FIXP_DBL *spec, int size, int inc, FIXP_TCC coeff [], int order)
{
  // - Simple all-pole filter of order "order" defined by
  //   y(n) =  x(n) - a(2)*y(n-1) - ... - a(order+1)*y(n-order)
  //
  // - The state variables of the filter are initialized to zero every time
  //
  // - The output data is written over the input data ("in-place operation")
  //
  // - An input vector of "size" samples is processed and the index increment
  //   to the next data sample is given by "inc"

  int i,j,N;
  FIXP_DBL *pSpec;
  FIXP_DBL maxVal=FL2FXCONST_DBL(0.0);
  INT s;

  FDK_ASSERT(order <= TNS_MAXIMUM_ORDER);
  C_ALLOC_SCRATCH_START(state, FIXP_DBL, TNS_MAXIMUM_ORDER);
  FDKmemclear(state, order*sizeof(FIXP_DBL));

  for (i=0; i<size; i++) {
    maxVal = fixMax(maxVal,fixp_abs(spec[i]));
  }

  if ( maxVal > FL2FXCONST_DBL(0.03125*0.70710678118) )
    s = fixMax(CntLeadingZeros(maxVal)-6,0);
  else
    s = fixMax(CntLeadingZeros(maxVal)-5,0);

  s = fixMin(s,2);
  s = s-1;

  if (inc == -1)
    pSpec = &spec[size - 1];
  else
    pSpec = &spec[0];

  FIXP_TCC *pCoeff;

#define FIRST_PART_FLTR                                              \
    FIXP_DBL x, *pState = state;                                     \
    pCoeff = coeff;                                                  \
                                                                     \
    if (s < 0)                                                       \
      x = (pSpec [0]>>1) + fMultDiv2 (*pCoeff++, pState [0]) ;       \
    else                                                             \
      x = (pSpec [0]<<s) + fMultDiv2 (*pCoeff++, pState [0]) ;

#define INNER_FLTR_INLINE                                            \
      x = fMultAddDiv2 (x, *pCoeff, pState [1]);                     \
      pState [0] = pState [1] - (fMultDiv2 (*pCoeff++, x) <<2) ;     \
      pState++;

#define LAST_PART_FLTR                                               \
      if (s < 0)                                                     \
        *pSpec = x << 1;                                             \
      else                                                           \
        *pSpec = x >> s;                                             \
      *pState =(-x) << 1;                                            \
      pSpec   += inc ;


   if (order>8)
   {
      N = (order-1)&7;

      for (i = size ; i != 0 ; i--)
      {
        FIRST_PART_FLTR

        for (j = N; j > 0 ; j--) { INNER_FLTR_INLINE }

        INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE
        INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE

        LAST_PART_FLTR
      }

   } else if (order>4) {

      N = (order-1)&3;

      for (i = size ; i != 0 ; i--)
      {
        FIRST_PART_FLTR
        for (j = N; j > 0 ; j--) { INNER_FLTR_INLINE }

        INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE

        LAST_PART_FLTR
      }

   } else {

      N = order-1;

      for (i = size ; i != 0 ; i--)
      {
        FIRST_PART_FLTR

        for (j = N; j > 0 ; j--) { INNER_FLTR_INLINE }

        LAST_PART_FLTR
      }
   }

   C_ALLOC_SCRATCH_END(state, FIXP_DBL, TNS_MAXIMUM_ORDER);
}

/*!
  \brief Apply tns to spectral lines

  The function applies the tns to the spectrum,

  \return  none
*/
void CTns_Apply (
        CTnsData *RESTRICT pTnsData, /*!< pointer to aac decoder info */
        const CIcsInfo *pIcsInfo,
        SPECTRAL_PTR pSpectralCoefficient,
        const SamplingRateInfo *pSamplingRateInfo,
        const INT granuleLength
        )
{
  int window,index,start,stop,size;


  if (pTnsData->Active)
  {
      C_AALLOC_SCRATCH_START(coeff, FIXP_TCC, TNS_MAXIMUM_ORDER);

      for (window=0; window < GetWindowsPerFrame(pIcsInfo); window++)
      {
        FIXP_DBL *pSpectrum = SPEC(pSpectralCoefficient, window, granuleLength);

        for (index=0; index < pTnsData->NumberOfFilters[window]; index++)
        {
          CFilter *RESTRICT filter = &pTnsData->Filter[window][index];

          if (filter->Order > 0)
          {
             FIXP_TCC *pCoeff;
             int tns_max_bands;

             pCoeff = &coeff[filter->Order-1];
             if (filter->Resolution == 3)
             {
               int i;
               for (i=0; i < filter->Order; i++)
                 *pCoeff-- = FDKaacDec_tnsCoeff3[filter->Coeff[i]+4];
             }
             else
             {
               int i;
               for (i=0; i < filter->Order; i++)
                 *pCoeff-- = FDKaacDec_tnsCoeff4[filter->Coeff[i]+8];
             }

             switch (granuleLength) {
               case 480:
                 tns_max_bands = tns_max_bands_tbl_480[pSamplingRateInfo->samplingRateIndex-3];
                 break;
               case 512:
                 tns_max_bands = tns_max_bands_tbl_512[pSamplingRateInfo->samplingRateIndex-3];
                 break;
               default:
                 tns_max_bands = GetMaximumTnsBands(pIcsInfo, pSamplingRateInfo->samplingRateIndex);
                 break;
             }

             start = fixMin( fixMin(filter->StartBand, tns_max_bands),
                             GetScaleFactorBandsTransmitted(pIcsInfo) );

             start = GetScaleFactorBandOffsets(pIcsInfo, pSamplingRateInfo)[start];

             stop = fixMin( fixMin(filter->StopBand, tns_max_bands),
                            GetScaleFactorBandsTransmitted(pIcsInfo) );

             stop = GetScaleFactorBandOffsets(pIcsInfo, pSamplingRateInfo)[stop];

             size = stop - start;

             if (size > 0) {
               CTns_Filter(&pSpectrum[start],
                            size,
                            filter->Direction,
                            coeff,
                            filter->Order );
             }
          }
        }
      }
      C_AALLOC_SCRATCH_END(coeff, FIXP_TCC, TNS_MAXIMUM_ORDER);
  }

}
