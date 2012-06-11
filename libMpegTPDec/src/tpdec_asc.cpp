
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2012 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/*****************************  MPEG-4 AAC Decoder  **************************

   Author(s):   Daniel Homm
   Description:

******************************************************************************/

#include "tpdec_lib.h"
#include "tp_data.h"


void CProgramConfig_Reset(CProgramConfig *pPce)
{
  pPce->elCounter = 0;
}

void CProgramConfig_Init(CProgramConfig *pPce)
{
  FDKmemclear(pPce, sizeof(CProgramConfig));
#ifdef TP_PCE_ENABLE
  pPce->SamplingFrequencyIndex = 0xf;
#endif
}

int  CProgramConfig_IsValid ( const CProgramConfig *pPce )
{
  return ( (pPce->isValid) ? 1 : 0);
}

#ifdef TP_PCE_ENABLE
void CProgramConfig_Read(
                          CProgramConfig *pPce,
                          HANDLE_FDK_BITSTREAM bs,
                          UINT alignmentAnchor
                        )
{
  int i;

  pPce->NumEffectiveChannels = 0;
  pPce->NumChannels = 0;
  pPce->ElementInstanceTag = (UCHAR) FDKreadBits(bs,4);
  pPce->Profile = (UCHAR) FDKreadBits(bs,2);
  pPce->SamplingFrequencyIndex = (UCHAR) FDKreadBits(bs,4);
  pPce->NumFrontChannelElements = (UCHAR) FDKreadBits(bs,4);
  pPce->NumSideChannelElements = (UCHAR) FDKreadBits(bs,4);
  pPce->NumBackChannelElements = (UCHAR) FDKreadBits(bs,4);
  pPce->NumLfeChannelElements = (UCHAR) FDKreadBits(bs,2);
  pPce->NumAssocDataElements = (UCHAR) FDKreadBits(bs,3);
  pPce->NumValidCcElements = (UCHAR) FDKreadBits(bs,4);

  if ((pPce->MonoMixdownPresent = (UCHAR) FDKreadBits(bs,1)) != 0)
  {
    pPce->MonoMixdownElementNumber = (UCHAR) FDKreadBits(bs,4);
  }

  if ((pPce->StereoMixdownPresent = (UCHAR) FDKreadBits(bs,1)) != 0)
  {
    pPce->StereoMixdownElementNumber = (UCHAR) FDKreadBits(bs,4);
  }

  if ((pPce->MatrixMixdownIndexPresent = (UCHAR) FDKreadBits(bs,1)) != 0)
  {
    pPce->MatrixMixdownIndex = (UCHAR) FDKreadBits(bs,2);
    pPce->PseudoSurroundEnable = (UCHAR) FDKreadBits(bs,1);
  }

  for (i=0; i < pPce->NumFrontChannelElements; i++)
  {
    pPce->FrontElementIsCpe[i] = (UCHAR) FDKreadBits(bs,1);
    pPce->FrontElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
    pPce->NumChannels += pPce->FrontElementIsCpe[i] ? 2 : 1;
  }

  for (i=0; i < pPce->NumSideChannelElements; i++)
  {
    pPce->SideElementIsCpe[i] = (UCHAR) FDKreadBits(bs,1);
    pPce->SideElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
    pPce->NumChannels += pPce->SideElementIsCpe[i] ? 2 : 1;
  }

  for (i=0; i < pPce->NumBackChannelElements; i++)
  {
    pPce->BackElementIsCpe[i] = (UCHAR) FDKreadBits(bs,1);
    pPce->BackElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
    pPce->NumChannels += pPce->BackElementIsCpe[i] ? 2 : 1;
  }

  pPce->NumEffectiveChannels = pPce->NumChannels;

  for (i=0; i < pPce->NumLfeChannelElements; i++)
  {
    pPce->LfeElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
    pPce->NumChannels += 1;
  }

  for (i=0; i < pPce->NumAssocDataElements; i++)
  {
    pPce->AssocDataElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
  }

  for (i=0; i < pPce->NumValidCcElements; i++)
  {
    pPce->CcElementIsIndSw[i] = (UCHAR) FDKreadBits(bs,1);
    pPce->ValidCcElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
  }

  FDKbyteAlign(bs, alignmentAnchor);

  pPce->CommentFieldBytes = (UCHAR) FDKreadBits(bs,8);

  for (i=0; i < pPce->CommentFieldBytes; i++)
  {
    UCHAR text;

    text = (UCHAR)FDKreadBits(bs,8);

    if (i < PC_COMMENTLENGTH)
    {
      pPce->Comment[i] = text;
    }
  }

  pPce->isValid = 1;
}
#endif /* TP_PCE_ENABLE */

/**
 * \brief get implicit audio channel type for given channelConfig and MPEG ordered channel index
 * \param channelConfig MPEG channelConfiguration from 1 upto 7
 * \param index MPEG channel order index
 * \return audio channel type.
 */
void getImplicitAudioChannelTypeAndIndex(
        AUDIO_CHANNEL_TYPE *chType,
        UCHAR *chIndex,
        UINT channelConfig,
        UINT index
        )
{
  if (index < 3) {
    *chType = ACT_FRONT;
    *chIndex = index;
  } else {
    switch (channelConfig) {
      case MODE_1_2_1:
      case MODE_1_2_2:
      case MODE_1_2_2_1:
        switch (index) {
          case 3:
          case 4:
            *chType = ACT_BACK;
            *chIndex = index - 3;
            break;
          case 5:
            *chType = ACT_LFE;
            *chIndex = 0;
            break;
        }
        break;
      case MODE_1_2_2_2_1:
        switch (index) {
          case 3:
          case 4:
            *chType = ACT_SIDE;
            *chIndex = index - 3;
            break;
          case 5:
          case 6:
            *chType = ACT_BACK;
            *chIndex = index - 5;
            break;
          case 7:
            *chType = ACT_LFE;
            *chIndex = 0;
            break;
        }
        break;
      default:
        *chType = ACT_NONE;
        break;
    }
  }
}

int CProgramConfig_LookupElement(
        CProgramConfig *pPce,
        const UINT      channelConfig,
        const UINT      tag,
        const UINT      channelIdx,
        UCHAR           chMapping[],
        AUDIO_CHANNEL_TYPE chType[],
        UCHAR           chIndex[],
        UCHAR          *elMapping,
        MP4_ELEMENT_ID  elList[],
        MP4_ELEMENT_ID  elType
       )
{
  if (channelConfig > 0)
  {
    /* Constant channel mapping must have
       been set during initialization. */
    if ( elType == ID_SCE
      || elType == ID_CPE
      || elType == ID_LFE )
    {
      *elMapping = pPce->elCounter;
      if (elList[pPce->elCounter] != elType) {
        /* Not in the list */
        return 0;
      }
      /* Assume all front channels */
      getImplicitAudioChannelTypeAndIndex(&chType[channelIdx], &chIndex[channelIdx], channelConfig, channelIdx);
      if (elType == ID_CPE) {
        chType[channelIdx+1] = chType[channelIdx];
        chIndex[channelIdx+1] = chIndex[channelIdx]+1;
      }
      pPce->elCounter++;
    }
    /* Accept all non-channel elements, too. */
    return 1;
  }
  else
  {
#ifdef TP_PCE_ENABLE
    if (!pPce->isValid)
#endif /* TP_PCE_ENABLE */
    {
      /* Implicit channel mapping. */
      if ( elType == ID_SCE
        || elType == ID_CPE
        || elType == ID_LFE )
      {
        /* Store all channel element IDs */
        elList[pPce->elCounter] = elType;
        *elMapping = pPce->elCounter++;
      }
    }
#ifdef  TP_PCE_ENABLE
    else {
      /* Accept the additional channel(s), only if the tag is in the lists */
      int isCpe = 0, i;
      int cc = 0, fc = 0, sc = 0, bc = 0, lc = 0, ec = 0; /* Channel and element counters */

      switch (elType)
      {
      case ID_CPE:
        isCpe = 1;
      case ID_SCE:
        /* search in front channels */
        for (i = 0; i < pPce->NumFrontChannelElements; i++) {
          if (isCpe == pPce->FrontElementIsCpe[i] && pPce->FrontElementTagSelect[i] == tag) {
            chMapping[cc] = channelIdx;
            chType[cc] = ACT_FRONT;
            chIndex[cc] = fc;
            if (isCpe) {
              chMapping[cc+1] = channelIdx+1;
              chType[cc+1] = ACT_FRONT;
              chIndex[cc+1] = fc+1;
            }
            *elMapping = ec;
            return 1;
          }
          ec++;
          if (pPce->FrontElementIsCpe[i]) {
            cc+=2; fc+=2;
          } else {
            cc++; fc++;
          }
        }
        /* search in side channels */
        for (i = 0; i < pPce->NumSideChannelElements; i++) {
          if (isCpe == pPce->SideElementIsCpe[i] && pPce->SideElementTagSelect[i] == tag) {
            chMapping[cc] = channelIdx;
            chType[cc] = ACT_SIDE;
            chIndex[cc] = sc;
            if (isCpe) {
              chMapping[cc+1] = channelIdx+1;
              chType[cc+1] = ACT_SIDE;
              chIndex[cc+1] = sc+1;
            }
            *elMapping = ec;
            return 1;
          }
          ec++;
          if (pPce->SideElementIsCpe[i]) {
            cc+=2; sc+=2;
          } else {
            cc++; sc++;
          }
        }
        /* search in back channels */
        for (i = 0; i < pPce->NumBackChannelElements; i++) {
          if (isCpe == pPce->BackElementIsCpe[i] && pPce->BackElementTagSelect[i] == tag) {
            chMapping[cc] = channelIdx;
            chType[cc] = ACT_BACK;
            chIndex[cc] = bc;
            if (isCpe) {
              chMapping[cc+1] = channelIdx+1;
              chType[cc+1] = ACT_BACK;
              chIndex[cc+1] = bc+1;
            }
            *elMapping = ec;
            return 1;
          }
          ec++;
          if (pPce->BackElementIsCpe[i]) {
            cc+=2; bc+=2;
          } else {
            cc++; bc++;
          }
        }
        break;

      case ID_LFE:
        /* Initialize channel counter and element counter */
        cc = pPce->NumEffectiveChannels;
        ec = pPce->NumFrontChannelElements+ pPce->NumSideChannelElements + pPce->NumBackChannelElements;
        /* search in lfe channels */
        for (i = 0; i < pPce->NumLfeChannelElements; i++) {
          if ( pPce->LfeElementTagSelect[i] == tag ) {
            chMapping[cc] = channelIdx;
            *elMapping = ec;
            chType[cc] = ACT_LFE;
            chIndex[cc] = lc;
            return 1;
          }
          ec++;
          cc++;
          lc++;
        }
        break;

      /* Non audio elements */
      case ID_CCE:
        /* search in cce channels */
        for (i = 0; i < pPce->NumValidCcElements; i++) {
          if (pPce->ValidCcElementTagSelect[i] == tag) {
            return 1;
          }
        }
        break;
      case ID_DSE:
        /* search associated data elements */
        for (i = 0; i < pPce->NumAssocDataElements; i++) {
          if (pPce->AssocDataElementTagSelect[i] == tag) {
            return 1;
          }
        }
        break;
      default:
        return 0;
      }
      return 0;  /* not found in any list */
    }
#endif /* TP_PCE_ENABLE */
  }

  return 1;
}

#ifdef  TP_PCE_ENABLE
int CProgramConfig_GetElementTable(
        const CProgramConfig *pPce,
        MP4_ELEMENT_ID  elList[],
        const INT elListSize
       )
{
  int i, el = 0;

  if ( elListSize 
    < pPce->NumFrontChannelElements + pPce->NumSideChannelElements + pPce->NumBackChannelElements + pPce->NumLfeChannelElements 
    )
  {
    return 0;
  }

  for (i=0; i < pPce->NumFrontChannelElements; i++)
  {
    elList[el++] = (pPce->FrontElementIsCpe[i]) ?  ID_CPE : ID_SCE;
  }

  for (i=0; i < pPce->NumSideChannelElements; i++)
  {
    elList[el++] = (pPce->SideElementIsCpe[i]) ?  ID_CPE : ID_SCE;
  }

  for (i=0; i < pPce->NumBackChannelElements; i++)
  {
    elList[el++] = (pPce->BackElementIsCpe[i]) ?  ID_CPE : ID_SCE;
  }

  for (i=0; i < pPce->NumLfeChannelElements; i++)
  {
    elList[el++] = ID_LFE;
  }


  return el;
}
#endif

static AUDIO_OBJECT_TYPE getAOT(HANDLE_FDK_BITSTREAM bs)
{
  int tmp = 0;

  tmp = FDKreadBits(bs,5);
  if (tmp == AOT_ESCAPE) {
    int tmp2 = FDKreadBits(bs,6);
    tmp = 32 + tmp2;
  }

  return (AUDIO_OBJECT_TYPE)tmp;
}

static INT getSampleRate(HANDLE_FDK_BITSTREAM bs, UCHAR *index, int nBits)
{
  INT sampleRate;
  int idx;

  idx = FDKreadBits(bs, nBits);
  if( idx == (1<<nBits)-1 ) {
    if(FDKgetValidBits(bs) < 24) {
      return 0;
    }
    sampleRate = FDKreadBits(bs,24);
  } else {
    sampleRate = SamplingRateTable[idx];
  }

  *index = idx;

  return sampleRate;
}

#ifdef TP_GA_ENABLE
static
TRANSPORTDEC_ERROR GaSpecificConfig_Parse( CSGaSpecificConfig    *self,
                                           CSAudioSpecificConfig *asc,
                                           HANDLE_FDK_BITSTREAM   bs,
                                           UINT                   ascStartAnchor )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;

  self->m_frameLengthFlag = FDKreadBits(bs,1);

  self->m_dependsOnCoreCoder = FDKreadBits(bs,1);

  if( self->m_dependsOnCoreCoder )
    self->m_coreCoderDelay = FDKreadBits(bs,14);

  self->m_extensionFlag = FDKreadBits(bs,1);

  if( asc->m_channelConfiguration == 0 ) {
    CProgramConfig_Read(&asc->m_progrConfigElement, bs, ascStartAnchor);
  }

  if ((asc->m_aot == AOT_AAC_SCAL) || (asc->m_aot == AOT_ER_AAC_SCAL)) {
    self->m_layer = FDKreadBits(bs,3);
  }

  if (self->m_extensionFlag) {
    if (asc->m_aot == AOT_ER_BSAC) {
      self->m_numOfSubFrame = FDKreadBits(bs,5);
      self->m_layerLength   = FDKreadBits(bs,11);
    }

    if ((asc->m_aot == AOT_ER_AAC_LC)   || (asc->m_aot == AOT_ER_AAC_LTP)  ||
        (asc->m_aot == AOT_ER_AAC_SCAL) || (asc->m_aot == AOT_ER_AAC_LD))
    {
      asc->m_vcb11Flag = FDKreadBits(bs,1); /* aacSectionDataResilienceFlag */
      asc->m_rvlcFlag  = FDKreadBits(bs,1); /* aacScalefactorDataResilienceFlag */
      asc->m_hcrFlag   = FDKreadBits(bs,1); /* aacSpectralDataResilienceFlag */
    }

    self->m_extensionFlag3 = FDKreadBits(bs,1);

  }
  return (ErrorStatus);
}
#endif /* TP_GA_ENABLE */





#ifdef TP_ELD_ENABLE

static INT ld_sbr_header( const CSAudioSpecificConfig *asc,
                           HANDLE_FDK_BITSTREAM hBs, 
                           CSTpCallBacks *cb )
{
  const int channelConfiguration = asc->m_channelConfiguration;
  int i = 0;
  INT error = 0;

  if (channelConfiguration == 2) {
    error = cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
  } else {
    error = cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_SCE, i++);
  }

  switch ( channelConfiguration ) {
    case 5:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
    case 3:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
      break;

    case 7:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_SCE, i++);
    case 6:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
    case 4:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
      break;
  }

  return error;
}

static
TRANSPORTDEC_ERROR EldSpecificConfig_Parse(
        CSAudioSpecificConfig *asc,
        HANDLE_FDK_BITSTREAM hBs,
        CSTpCallBacks *cb
        )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;
  CSEldSpecificConfig *esc = &asc->m_sc.m_eldSpecificConfig;
  ASC_ELD_EXT_TYPE eldExtType;
  int eldExtLen, len, cnt;

  FDKmemclear(esc, sizeof(CSEldSpecificConfig));

  esc->m_frameLengthFlag = FDKreadBits(hBs, 1 );
  if (esc->m_frameLengthFlag) {
    asc->m_samplesPerFrame = 480;
  } else {
    asc->m_samplesPerFrame = 512;
  }

  asc->m_vcb11Flag = FDKreadBits(hBs, 1 );
  asc->m_rvlcFlag  = FDKreadBits(hBs, 1 );
  asc->m_hcrFlag   = FDKreadBits(hBs, 1 );

  esc->m_sbrPresentFlag     = FDKreadBits(hBs, 1 );

  if (esc->m_sbrPresentFlag == 1) {
    esc->m_sbrSamplingRate    = FDKreadBits(hBs, 1 ); /* 0: single rate, 1: dual rate */
    esc->m_sbrCrcFlag         = FDKreadBits(hBs, 1 );

    asc->m_extensionSamplingFrequency = asc->m_samplingFrequency << esc->m_sbrSamplingRate;

    if (cb->cbSbr != NULL){
      if ( 0 != ld_sbr_header(asc, hBs, cb) ) {
        return TRANSPORTDEC_PARSE_ERROR;
      }
    }
  }
  esc->m_useLdQmfTimeAlign = 0;

  /* new ELD syntax */
  /* parse ExtTypeConfigData */
  while ((eldExtType = (ASC_ELD_EXT_TYPE)FDKreadBits(hBs, 4 )) != ELDEXT_TERM) {
    eldExtLen = len = FDKreadBits(hBs, 4 );
    if ( len == 0xf ) {
      len = FDKreadBits(hBs, 8 );
      eldExtLen += len;

      if ( len == 0xff ) {
        len = FDKreadBits(hBs, 16 );
        eldExtLen += len;
      }
    }

    switch (eldExtType) {
      case ELDEXT_LDSAC:
        esc->m_useLdQmfTimeAlign = 1;
        if (cb->cbSsc != NULL) {
          ErrorStatus = (TRANSPORTDEC_ERROR)cb->cbSsc(
                  cb->cbSscData,
                  hBs,
                  asc->m_aot,
                  asc->m_samplingFrequency,
                  1,  /* muxMode */
                  len
                  );
        } else {
          ErrorStatus = TRANSPORTDEC_UNSUPPORTED_FORMAT;
        }
        if (ErrorStatus != TRANSPORTDEC_OK) {
          goto bail;
        }
        break;
      default:
        for(cnt=0; cnt<len; cnt++) {
          FDKreadBits(hBs, 8 );
        }
        break;
      /* add future eld extension configs here */
    }
  }
bail:
  return (ErrorStatus);
}
#endif /* TP_ELD_ENABLE */



/*
 * API Functions
 */

void AudioSpecificConfig_Init(CSAudioSpecificConfig *asc)
{
  FDKmemclear(asc, sizeof(CSAudioSpecificConfig));

  /* Init all values that should not be zero. */
  asc->m_aot                    = AOT_NONE;
  asc->m_samplingFrequencyIndex = 0xf;
  asc->m_epConfig               = -1;
  asc->m_extensionAudioObjectType        = AOT_NULL_OBJECT;
#ifdef TP_PCE_ENABLE
  CProgramConfig_Init(&asc->m_progrConfigElement);
#endif
}

TRANSPORTDEC_ERROR AudioSpecificConfig_Parse(
        CSAudioSpecificConfig *self,
        HANDLE_FDK_BITSTREAM   bs,
        int                    fExplicitBackwardCompatible,
        CSTpCallBacks      *cb
        )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;
  UINT ascStartAnchor = FDKgetValidBits(bs);
  int frameLengthFlag = -1;

  AudioSpecificConfig_Init(self);

  self->m_aot = getAOT(bs);
  self->m_samplingFrequency = getSampleRate(bs, &self->m_samplingFrequencyIndex, 4);
  if (self->m_samplingFrequency <= 0) {
    return TRANSPORTDEC_PARSE_ERROR;
  }

  self->m_channelConfiguration = FDKreadBits(bs,4);

  /* SBR extension ( explicit non-backwards compatible mode ) */
  self->m_sbrPresentFlag = 0;
  self->m_psPresentFlag  = 0;

  if ( self->m_aot == AOT_SBR || self->m_aot == AOT_PS ) {
    self->m_extensionAudioObjectType = AOT_SBR;

    self->m_sbrPresentFlag = 1;
    if ( self->m_aot == AOT_PS ) {
      self->m_psPresentFlag = 1;
    }

    self->m_extensionSamplingFrequency = getSampleRate(bs, &self->m_extensionSamplingFrequencyIndex, 4);
    self->m_aot = getAOT(bs);

  } else {
    self->m_extensionAudioObjectType = AOT_NULL_OBJECT;
  }

  /* Parse whatever specific configs */
  switch (self->m_aot)
  {
#ifdef TP_GA_ENABLE
    case AOT_AAC_LC:
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LD:
    case AOT_ER_AAC_SCAL:
    case AOT_ER_BSAC:
      if ((ErrorStatus = GaSpecificConfig_Parse(&self->m_sc.m_gaSpecificConfig, self, bs, ascStartAnchor)) != TRANSPORTDEC_OK ) {
        return (ErrorStatus);
      }
      frameLengthFlag = self->m_sc.m_gaSpecificConfig.m_frameLengthFlag;
      break;
#endif /* TP_GA_ENABLE */
    case AOT_MPEGS:
      if (cb->cbSsc != NULL) {
        cb->cbSsc(
                cb->cbSscData,
                bs,
                self->m_aot,
                self->m_samplingFrequency,
                1,
                0  /* don't know the length */
                );
      } else {
        return TRANSPORTDEC_UNSUPPORTED_FORMAT;
      }
      break;
#ifdef TP_ELD_ENABLE
    case AOT_ER_AAC_ELD:
      if ((ErrorStatus = EldSpecificConfig_Parse(self, bs, cb)) != TRANSPORTDEC_OK ) {
        return (ErrorStatus);
      }
      frameLengthFlag = self->m_sc.m_eldSpecificConfig.m_frameLengthFlag;
      self->m_sbrPresentFlag = self->m_sc.m_eldSpecificConfig.m_sbrPresentFlag;
      self->m_extensionSamplingFrequency = (self->m_sc.m_eldSpecificConfig.m_sbrSamplingRate+1) * self->m_samplingFrequency;
      break;
#endif /* TP_ELD_ENABLE */

    default:
      return TRANSPORTDEC_UNSUPPORTED_FORMAT;
      break;
  }

  /* Frame length */
  switch (self->m_aot)
  {
#if defined(TP_GA_ENABLE) || defined(TP_USAC_ENABLE)
    case AOT_AAC_LC:
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_SCAL:
    case AOT_ER_BSAC:
    /*case AOT_USAC:*/
      if (!frameLengthFlag)
        self->m_samplesPerFrame = 1024;
      else
        self->m_samplesPerFrame = 960;
      break;
#endif /* TP_GA_ENABLE */
#if defined(TP_GA_ENABLE)
    case AOT_ER_AAC_LD:
      if (!frameLengthFlag)
        self->m_samplesPerFrame = 512;
      else
        self->m_samplesPerFrame = 480;
      break;
#endif /* defined(TP_GA_ENABLE) */
    default:
      break;
  }

  switch (self->m_aot)
  {
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LD:
    case AOT_ER_AAC_ELD:
    case AOT_ER_AAC_SCAL:
    case AOT_ER_CELP:
    case AOT_ER_HVXC:
    case AOT_ER_BSAC:
      self->m_epConfig = FDKreadBits(bs,2);

      if (self->m_epConfig > 1) {
        return TRANSPORTDEC_UNSUPPORTED_FORMAT; // EPCONFIG;
      }
      break;
    default:
      break;
  }


  return (ErrorStatus);
}


