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
   Description: joint stereo processing

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "stereo.h"


#include "aac_rom.h"
#include "FDK_bitstream.h"
#include "channelinfo.h"

enum
{
  L = 0,
  R = 1
};


int CJointStereo_Read(
        HANDLE_FDK_BITSTREAM bs,
        CJointStereoData *pJointStereoData,
        const int windowGroups,
        const int scaleFactorBandsTransmitted,
        const UINT flags
        )
{
  int group,band;

  pJointStereoData->MsMaskPresent = (UCHAR) FDKreadBits(bs,2);

  FDKmemclear(pJointStereoData->MsUsed, scaleFactorBandsTransmitted*sizeof(UCHAR));

  switch (pJointStereoData->MsMaskPresent)
  {
    case 0 : /* no M/S */
      /* all flags are already cleared */
      break ;

    case 1 : /* read ms_used */

      for (group=0; group<windowGroups; group++)
      {
        for (band=0; band<scaleFactorBandsTransmitted; band++)
        {
          pJointStereoData->MsUsed[band] |= (FDKreadBits(bs,1) << group);
        }
      }
      break ;

    case 2 : /* full spectrum M/S */

      for (band=0; band<scaleFactorBandsTransmitted; band++)
      {
        pJointStereoData->MsUsed[band] = 255 ;  /* set all flags to 1 */
      }
      break ;
  }

  return 0;
}

void CJointStereo_ApplyMS(
        CAacDecoderChannelInfo *pAacDecoderChannelInfo[2],
        const SHORT *pScaleFactorBandOffsets,
        const UCHAR *pWindowGroupLength,
        const int windowGroups,
        const int scaleFactorBandsTransmittedL,
        const int scaleFactorBandsTransmittedR
        )
{
  CJointStereoData *pJointStereoData = &pAacDecoderChannelInfo[L]->pComData->jointStereoData;
  int window, group, scaleFactorBandsTransmitted;

  FDK_ASSERT(scaleFactorBandsTransmittedL == scaleFactorBandsTransmittedR);
  scaleFactorBandsTransmitted = scaleFactorBandsTransmittedL;
  for (window = 0, group = 0; group < windowGroups; group++)
  {
    UCHAR groupMask = 1 << group;

    for (int groupwin=0; groupwin<pWindowGroupLength[group]; groupwin++, window++)
    {
      int band;
      FIXP_DBL *leftSpectrum, *rightSpectrum;
      SHORT *leftScale = &pAacDecoderChannelInfo[L]->pDynData->aSfbScale[window*16];
      SHORT *rightScale = &pAacDecoderChannelInfo[R]->pDynData->aSfbScale[window*16];

      leftSpectrum = SPEC(pAacDecoderChannelInfo[L]->pSpectralCoefficient, window, pAacDecoderChannelInfo[L]->granuleLength);
      rightSpectrum = SPEC(pAacDecoderChannelInfo[R]->pSpectralCoefficient, window, pAacDecoderChannelInfo[R]->granuleLength);

      for (band=0; band<scaleFactorBandsTransmitted; band++)
      {
        if (pJointStereoData->MsUsed[band] & groupMask)
        {
          int lScale=leftScale[band];
          int rScale=rightScale[band];
          int commonScale=lScale > rScale ? lScale:rScale;

          /* ISO/IEC 14496-3 Chapter 4.6.8.1.1 :
             M/S joint channel coding can only be used if common_window is ‘1’. */
          FDK_ASSERT(GetWindowSequence(&pAacDecoderChannelInfo[L]->icsInfo) == GetWindowSequence(&pAacDecoderChannelInfo[R]->icsInfo));
          FDK_ASSERT(GetWindowShape(&pAacDecoderChannelInfo[L]->icsInfo) == GetWindowShape(&pAacDecoderChannelInfo[R]->icsInfo));

          commonScale++;
          leftScale[band]=commonScale;
          rightScale[band]=commonScale;

          lScale = fMin(DFRACT_BITS-1, commonScale - lScale);
          rScale = fMin(DFRACT_BITS-1, commonScale - rScale);

          FDK_ASSERT(lScale >= 0 && rScale >= 0);

          for (int index=pScaleFactorBandOffsets[band]; index<pScaleFactorBandOffsets[band+1]; index++)
          {
            FIXP_DBL leftCoefficient  = leftSpectrum [index] ;
            FIXP_DBL rightCoefficient = rightSpectrum [index] ;

            leftCoefficient >>= lScale ;
            rightCoefficient >>= rScale ;

            leftSpectrum [index] = leftCoefficient + rightCoefficient ;
            rightSpectrum [index] = leftCoefficient - rightCoefficient ;
          }
        }
      }
    }
  }

  /* Reset MsUsed flags if no explicit signalling was transmitted. Necessary for intensity coding.
     PNS correlation signalling was mapped before calling CJointStereo_ApplyMS(). */
  if (pJointStereoData->MsMaskPresent == 2) {
    FDKmemclear(pJointStereoData->MsUsed, JointStereoMaximumBands * sizeof(UCHAR));
  }
}

void CJointStereo_ApplyIS(
        CAacDecoderChannelInfo *pAacDecoderChannelInfo[2],
        const SHORT *pScaleFactorBandOffsets,
        const UCHAR *pWindowGroupLength,
        const int windowGroups,
        const int scaleFactorBandsTransmitted,
        const UINT CommonWindow
        )
{
  CJointStereoData *pJointStereoData = &pAacDecoderChannelInfo[L]->pComData->jointStereoData;

  for (int window=0,group=0; group<windowGroups; group++)
  {
    UCHAR *CodeBook;
    SHORT *ScaleFactor;
    UCHAR groupMask = 1 << group;

    CodeBook = &pAacDecoderChannelInfo[R]->pDynData->aCodeBook[group*16];
    ScaleFactor = &pAacDecoderChannelInfo[R]->pDynData->aScaleFactor[group*16];

    for (int groupwin=0; groupwin<pWindowGroupLength[group]; groupwin++, window++)
    {
      FIXP_DBL *leftSpectrum, *rightSpectrum;
      SHORT *leftScale = &pAacDecoderChannelInfo[L]->pDynData->aSfbScale[window*16];
      SHORT *rightScale = &pAacDecoderChannelInfo[R]->pDynData->aSfbScale[window*16];
      int band;

      leftSpectrum = SPEC(pAacDecoderChannelInfo[L]->pSpectralCoefficient, window, pAacDecoderChannelInfo[L]->granuleLength);
      rightSpectrum = SPEC(pAacDecoderChannelInfo[R]->pSpectralCoefficient, window, pAacDecoderChannelInfo[R]->granuleLength);

      for (band=0; band<scaleFactorBandsTransmitted; band++)
      {
        if ((CodeBook [band] == INTENSITY_HCB) ||
            (CodeBook [band] == INTENSITY_HCB2))
        {
          int bandScale = -(ScaleFactor [band] + 100) ;

          int msb = bandScale >> 2 ;
          int lsb = bandScale & 0x03 ;

          /* exponent of MantissaTable[lsb][0] is 1, thus msb+1 below. */
          FIXP_DBL scale = MantissaTable[lsb][0];

          /* ISO/IEC 14496-3 Chapter 4.6.8.2.3 :
             The use of intensity stereo coding is signaled by the use of the pseudo codebooks
             INTENSITY_HCB and INTENSITY_HCB2 (15 and 14) only in the right channel of a
             channel_pair_element() having a common ics_info() (common_window == 1). */
          FDK_ASSERT(GetWindowSequence(&pAacDecoderChannelInfo[L]->icsInfo) == GetWindowSequence(&pAacDecoderChannelInfo[R]->icsInfo));
          FDK_ASSERT(GetWindowShape(&pAacDecoderChannelInfo[L]->icsInfo) == GetWindowShape(&pAacDecoderChannelInfo[R]->icsInfo));

          rightScale[band] = leftScale[band]+msb+1;

          if (CommonWindow && (pJointStereoData->MsUsed[band] & groupMask))
          {

            if (CodeBook[band] == INTENSITY_HCB) /* _NOT_ in-phase */
            {
              scale = -scale ;
            }
          }
          else
          {
            if (CodeBook[band] == INTENSITY_HCB2) /* out-of-phase */
            {
              scale = -scale ;
            }
          }

          for (int index=pScaleFactorBandOffsets[band]; index<pScaleFactorBandOffsets[band+1]; index++)
          {
            rightSpectrum[index] = fMult(leftSpectrum[index],scale);
          }
        }
      }
    }
  }
}
