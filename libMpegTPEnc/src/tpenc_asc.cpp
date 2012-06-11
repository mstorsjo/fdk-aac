
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

/*****************************  MPEG-4 AAC Encoder  **************************

   Author(s):
   Description:

******************************************************************************/

#include "tp_data.h"

#include "tpenc_lib.h"
#include "tpenc_asc.h"
#include "FDK_bitstream.h"
#include "genericStds.h"

#define PCE_MAX_ELEMENTS 8

/**
 *  Describe a PCE based on placed channel elements and element type sequence.
 */
typedef struct {

    UCHAR    num_front_channel_elements;     /*!< Number of front channel elements. */
    UCHAR    num_side_channel_elements;      /*!< Number of side channel elements. */
    UCHAR    num_back_channel_elements;      /*!< Number of back channel elements. */
    UCHAR    num_lfe_channel_elements;       /*!< Number of lfe channel elements. */
    MP4_ELEMENT_ID el_list[PCE_MAX_ELEMENTS];/*!< List contains sequence describing the elements
                                                  in present channel mode. (MPEG order) */
} PCE_CONFIGURATION;


/**
 *  Map an incoming channel mode to a existing PCE configuration entry.
 */
typedef struct {

    CHANNEL_MODE        channel_mode;        /*!< Present channel mode. */
    PCE_CONFIGURATION   pce_configuration;   /*!< Program config element description. */

} CHANNEL_CONFIGURATION;


/**
 * \brief Table contains all supported channel modes and according PCE configuration description.
 *
 * The number of channel element parameter describes the kind of consecutively elements.
 * E.g. MODE_1_2_2_2_1 means:
 *                          - First 2 elements (SCE,CPE) are front channel elements.
 *                          - Following element (CPE) is a side channel element.
 *                          - Next element (CPE) is a back channel element.
 *                          - Last element (LFE) is a lfe channel element.
 */
static const CHANNEL_CONFIGURATION pceConfigTab[] =
{
  { MODE_1,                        {  1, 0, 0, 0, { ID_SCE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_2,                        {  1, 0, 0, 0, { ID_CPE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_1_2,                      {  2, 0, 0, 0, { ID_SCE,  ID_CPE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_1_2_1,                    {  2, 0, 1, 0, { ID_SCE,  ID_CPE,  ID_SCE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_1_2_2,                    {  2, 0, 1, 0, { ID_SCE,  ID_CPE,  ID_CPE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_1_2_2_1,                  {  2, 0, 1, 1, { ID_SCE,  ID_CPE,  ID_CPE,  ID_LFE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_1_2_2_2_1,                {  2, 1, 1, 1, { ID_SCE,  ID_CPE,  ID_CPE,  ID_CPE,  ID_LFE, ID_NONE, ID_NONE, ID_NONE } } },

  { MODE_1_1,                      {  2, 0, 0, 0, { ID_SCE,  ID_SCE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_1_1_1_1,                  {  2, 2, 0, 0, { ID_SCE,  ID_SCE,  ID_SCE,  ID_SCE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_1_1_1_1_1_1,              {  2, 2, 2, 0, { ID_SCE,  ID_SCE,  ID_SCE,  ID_SCE,  ID_SCE,  ID_SCE,  ID_NONE, ID_NONE } } },
  { MODE_1_1_1_1_1_1_1_1,          {  3, 2, 3, 0, { ID_SCE,  ID_SCE,  ID_SCE,  ID_SCE,  ID_SCE,  ID_SCE,  ID_SCE,  ID_SCE } } },

  { MODE_2_2,                      {  1, 0, 1, 0, { ID_CPE,  ID_CPE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_2_2_2,                    {  1, 1, 1, 0, { ID_CPE,  ID_CPE,  ID_CPE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },
  { MODE_2_2_2_2,                  {  4, 0, 0, 0, { ID_CPE,  ID_CPE,  ID_CPE,  ID_CPE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE } } },

  { MODE_2_1,                      {  1, 0, 1, 0, { ID_CPE,  ID_SCE,  ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE } } }
};


/**
 * \brief  Get program config element description for existing channel mode.
 *
 * \param channel_mode          Current channel mode.
 *
 * \return
 *          - Pointer to PCE_CONFIGURATION entry, on success.
 *          - NULL, on failure.
 */
static const PCE_CONFIGURATION* getPceEntry(
        const CHANNEL_MODE channel_mode
        )
{
  UINT i;
  const PCE_CONFIGURATION *pce_config = NULL;

  for (i=0; i < (sizeof(pceConfigTab)/sizeof(CHANNEL_CONFIGURATION)); i++) {
    if (pceConfigTab[i].channel_mode == channel_mode) {
      pce_config = &pceConfigTab[i].pce_configuration;
    }
  }

  return pce_config;
}

int getChannelConfig( CHANNEL_MODE channel_mode )
{
  INT chan_config = 0;

  switch(channel_mode) {
    case MODE_1:         chan_config = 1; break;
    case MODE_2:         chan_config = 2; break;
    case MODE_1_2:       chan_config = 3; break;
    case MODE_1_2_1:     chan_config = 4; break;
    case MODE_1_2_2:     chan_config = 5; break;
    case MODE_1_2_2_1:   chan_config = 6; break;
    case MODE_1_2_2_2_1: chan_config = 7; break;

    default:             chan_config = 0;
  }

  return chan_config;
}

CHANNEL_MODE transportEnc_GetChannelMode( int noChannels )
{
  CHANNEL_MODE chMode;

  if (noChannels <= 8 && noChannels > 0)
    chMode = (CHANNEL_MODE)((noChannels == 8) ? 7 : noChannels); /* see : iso/mpeg4 v1 audio subpart1*/
  else
    chMode = MODE_UNKNOWN;

  return chMode;
}

#ifdef TP_PCE_ENABLE
int transportEnc_writePCE(HANDLE_FDK_BITSTREAM hBs,
                          CHANNEL_MODE channelMode,
                          INT sampleRate,
                          int instanceTagPCE,
                          int profile,
                          int matrixMixdownA,
                          int pseudoSurroundEnable,
                          UINT alignAnchor)
{
  int sampleRateIndex, i;
  const PCE_CONFIGURATION* config = NULL;
  const MP4_ELEMENT_ID* pEl_list = NULL;
  UCHAR cpeCnt=0, sceCnt=0, lfeCnt=0;

  sampleRateIndex = getSamplingRateIndex(sampleRate);
  if (sampleRateIndex == 15) {
    return -1;
  }

  if ((config=getPceEntry(channelMode))==NULL) {
    return -1;
  }

  /* Pointer to first element in element list. */
  pEl_list = &config->el_list[0];

  FDKwriteBits(hBs, instanceTagPCE,  4);                        /* Element instance tag */
  FDKwriteBits(hBs, profile,         2);                        /* Object type */
  FDKwriteBits(hBs, sampleRateIndex, 4);                        /* Sample rate index*/

  FDKwriteBits(hBs, config->num_front_channel_elements, 4);     /* Front channel Elements */
  FDKwriteBits(hBs, config->num_side_channel_elements , 4);     /* No Side Channel Elements */
  FDKwriteBits(hBs, config->num_back_channel_elements , 4);     /* No Back channel Elements */
  FDKwriteBits(hBs, config->num_lfe_channel_elements  , 2);     /* No Lfe channel elements */

  FDKwriteBits(hBs, 0, 3);                                      /* No assoc data elements */
  FDKwriteBits(hBs, 0, 4);                                      /* No valid cc elements */
  FDKwriteBits(hBs, 0, 1);                                      /* Mono mixdown present */
  FDKwriteBits(hBs, 0, 1);                                      /* Stereo mixdown present */

  if ( matrixMixdownA!=0 && ((channelMode==MODE_1_2_2)||(channelMode==MODE_1_2_2_1)) ) {
      FDKwriteBits(hBs, 1, 1);                                  /* Matrix mixdown present */
      FDKwriteBits(hBs, (matrixMixdownA-1)&0x3, 2);             /* matrix_mixdown_idx */
      FDKwriteBits(hBs, pseudoSurroundEnable&0x1, 1);           /* pseudo_surround_enable */
  }
  else {
      FDKwriteBits(hBs, 0, 1);                                  /* Matrix mixdown not present */
  }

  for(i=0; i<config->num_front_channel_elements; i++) {
      UCHAR isCpe = (*pEl_list++==ID_CPE) ? 1 : 0;
      UCHAR tag   = (isCpe) ? cpeCnt++ : sceCnt++;
      FDKwriteBits(hBs, isCpe, 1);                              /* Front channel Elements is CPE? */
      FDKwriteBits(hBs, tag, 4);                                /* Front channel Instance Tag.*/
  }
  for(i=0; i<config->num_side_channel_elements; i++) {
      UCHAR isCpe = (*pEl_list++==ID_CPE) ? 1 : 0;
      UCHAR tag   = (isCpe) ? cpeCnt++ : sceCnt++;
      FDKwriteBits(hBs, isCpe, 1);                              /* Front channel Elements is CPE? */
      FDKwriteBits(hBs, tag, 4);                                /* Front channel Instance Tag.*/
  }
  for(i=0; i<config->num_back_channel_elements; i++) {
      UCHAR isCpe = (*pEl_list++==ID_CPE) ? 1 : 0;
      UCHAR tag   = (isCpe) ? cpeCnt++ : sceCnt++;
      FDKwriteBits(hBs, isCpe, 1);                              /* Front channel Elements is CPE? */
      FDKwriteBits(hBs, tag, 4);                                /* Front channel Instance Tag.*/
  }
  for(i=0; i<config->num_lfe_channel_elements; i++) {
      FDKwriteBits(hBs, lfeCnt++, 4);                           /* LFE channel Instance Tag. */
  }

  /* - num_valid_cc_elements always 0.
     - num_assoc_data_elements always 0. */

  /* Byte alignment: relative to alignAnchor
       ADTS: align with respect to the first bit of the raw_data_block()
       ADIF: align with respect to the first bit of the header
       LATM: align with respect to the first bit of the ASC */
  FDKbyteAlign(hBs, alignAnchor);                               /* Alignment */

  FDKwriteBits(hBs, 0 ,8);                                      /* Do no write any comment. */

  /* - comment_field_bytes always 0. */

  return 0;
}

int transportEnc_GetPCEBits(CHANNEL_MODE channelMode,
                            int matrixMixdownA,
                            int bits)
{
  const PCE_CONFIGURATION* config = NULL;

  if ((config=getPceEntry(channelMode))==NULL) {
    return -1;  /* unsupported channelmapping */
  }

  bits += 4 + 2 + 4;        /* Element instance tag  + Object type + Sample rate index */
  bits += 4 + 4 + 4 + 2;    /* No (front + side + back + lfe channel) elements */
  bits += 3 + 4;            /* No (assoc data + valid cc) elements */
  bits += 1 + 1 + 1 ;       /* Mono + Stereo + Matrix mixdown present */

  if ( matrixMixdownA!=0 && ((channelMode==MODE_1_2_2)||(channelMode==MODE_1_2_2_1)) ) {
    bits +=3;               /* matrix_mixdown_idx + pseudo_surround_enable */
  }

  bits += (1+4) * (INT)config->num_front_channel_elements;
  bits += (1+4) * (INT)config->num_side_channel_elements;
  bits += (1+4) * (INT)config->num_back_channel_elements;
  bits +=   (4) * (INT)config->num_lfe_channel_elements;

  /* - num_valid_cc_elements always 0.
     - num_assoc_data_elements always 0. */

  if ((bits%8) != 0) {
    bits += (8 - (bits%8)); /* Alignment */
  }

  bits += 8;                /* Comment field  bytes */

  /* - comment_field_bytes alwys 0. */

  return bits;
}
#endif /* TP_PCE_ENABLE */

static void writeAot(HANDLE_FDK_BITSTREAM hBitstreamBuffer, AUDIO_OBJECT_TYPE aot)
{
    int tmp = (int) aot;

    if (tmp > 31) {
        FDKwriteBits( hBitstreamBuffer, AOT_ESCAPE, 5 );
        FDKwriteBits( hBitstreamBuffer, tmp-32, 6 );   /* AudioObjectType */
    } else {
        FDKwriteBits( hBitstreamBuffer, tmp, 5 );
    }
}

static void writeSampleRate(HANDLE_FDK_BITSTREAM hBitstreamBuffer, int sampleRate)
{
  int sampleRateIndex = getSamplingRateIndex(sampleRate);

  FDKwriteBits( hBitstreamBuffer, sampleRateIndex, 4 );
  if( sampleRateIndex == 15 ) {
    FDKwriteBits( hBitstreamBuffer, sampleRate, 24 );
  }
}

#ifdef TP_GA_ENABLE
static
int transportEnc_writeGASpecificConfig(
                                        HANDLE_FDK_BITSTREAM asc,
                                        CODER_CONFIG *config,
                                        int          extFlg,
                                        UINT         alignAnchor
                                       )
{
  int aot = config->aot;
  int samplesPerFrame = config->samplesPerFrame;

  /* start of GASpecificConfig according to ISO/IEC 14496-3 Subpart 4, 4.4.1 */
  FDKwriteBits( asc, ((samplesPerFrame==960 || samplesPerFrame==480)?1:0), 1);  /* frameLengthFlag: 1 for a 960/480 (I)MDCT, 0 for a 1024/512 (I)MDCT*/
  FDKwriteBits( asc, 0, 1);  /* dependsOnCoreCoder: Sampling Rate Coder Specific, see in ISO/IEC 14496-3 Subpart 4, 4.4.1 */
  FDKwriteBits( asc, extFlg, 1 ); /* Extension Flag: Shall be 1 for aot = 17,19,20,21,22,23 */

  /* Write PCE if channel config is not 1-7 */
  if (getChannelConfig(config->channelMode) == 0) {
      transportEnc_writePCE(asc, config->channelMode, config->samplingRate, 0, 1, 0, 0, alignAnchor);
  }
  if (extFlg) {
    if (aot == AOT_ER_BSAC) {
      FDKwriteBits( asc, config->BSACnumOfSubFrame, 5 ); /* numOfSubFrame */
      FDKwriteBits( asc, config->BSAClayerLength, 11 );  /* layer_length */
    }
    if ((aot == AOT_ER_AAC_LC)   || (aot == AOT_ER_AAC_LTP)  ||
        (aot == AOT_ER_AAC_SCAL) || (aot == AOT_ER_AAC_LD))
    {
      FDKwriteBits( asc, (config->flags & CC_VCB11) ? 1 : 0, 1 ); /* aacSectionDataResillienceFlag */
      FDKwriteBits( asc, (config->flags & CC_RVLC) ? 1 : 0,  1 ); /* aacScaleFactorDataResillienceFlag */
      FDKwriteBits( asc, (config->flags & CC_HCR) ? 1 : 0,   1 ); /* aacSpectralDataResillienceFlag */
    }
    FDKwriteBits( asc, 0, 1 ); /* extensionFlag3: reserved. Shall be '0' */
  }
  return 0;
}
#endif /* TP_GA_ENABLE */

#ifdef TP_ELD_ENABLE

static
int transportEnc_writeELDSpecificConfig(
                                         HANDLE_FDK_BITSTREAM hBs,
                                         CODER_CONFIG *config,
                                         int        epConfig,
                                         CSTpCallBacks *cb
                                        )
{
  /* ELD specific config */
  if (config->channelMode == MODE_1_1) {
    return -1;
  }
  FDKwriteBits(hBs, (config->samplesPerFrame == 480) ? 1 : 0, 1);

  FDKwriteBits(hBs, (config->flags & CC_VCB11 ) ? 1:0, 1);
  FDKwriteBits(hBs, (config->flags & CC_RVLC ) ? 1:0, 1);
  FDKwriteBits(hBs, (config->flags & CC_HCR  ) ? 1:0, 1);

  FDKwriteBits(hBs, (config->flags & CC_SBR) ? 1:0, 1); /* SBR header flag */
  if ( (config->flags & CC_SBR) ) {
    FDKwriteBits(hBs, (config->samplingRate == config->extSamplingRate) ? 0:1, 1); /* Samplerate Flag */
    FDKwriteBits(hBs, (config->flags & CC_SBRCRC) ? 1:0, 1); /* SBR CRC flag*/

    if (cb->cbSbr != NULL) {
      const PCE_CONFIGURATION *pPce;
      int e;

      pPce = getPceEntry(config->channelMode);

      for (e=0; e<PCE_MAX_ELEMENTS && pPce->el_list[e] != ID_NONE; e++  ) {
        if ( (pPce->el_list[e] == ID_SCE) || (pPce->el_list[e] == ID_CPE) ) {
          cb->cbSbr(cb->cbSbrData, hBs, 0, 0, 0, config->aot, pPce->el_list[e], e);
        }
      }
    }
  }

  FDKwriteBits(hBs, 0, 4); /* ELDEXT_TERM */

  return 0;
}
#endif /* TP_ELD_ENABLE */


int transportEnc_writeASC (
                            HANDLE_FDK_BITSTREAM asc,
                            CODER_CONFIG *config,
                            CSTpCallBacks *cb
                           )
{
  UINT extFlag = 0;
  int err;
  int epConfig = 0;

  /* Required for the PCE. */
  UINT alignAnchor = FDKgetValidBits(asc);

  /* Extension Flag: Shall be 1 for aot = 17,19,20,21,22,23,39 */
  switch (config->aot) {
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LTP:
    case AOT_ER_AAC_SCAL:
    case AOT_ER_TWIN_VQ:
    case AOT_ER_BSAC:
    case AOT_ER_AAC_LD:
    case AOT_ER_AAC_ELD:
    case AOT_USAC:
        extFlag = 1;
        break;
    default:
        break;
  }

  if (config->extAOT == AOT_SBR || config->extAOT == AOT_PS)
    writeAot(asc, config->extAOT);
  else
    writeAot(asc, config->aot);

  {
    writeSampleRate(asc, config->samplingRate);
  }

  /* Try to guess a reasonable channel mode if not given */
  if (config->channelMode == MODE_INVALID) {
    config->channelMode = transportEnc_GetChannelMode(config->noChannels);
    if (config->channelMode == MODE_INVALID)
      return -1;
  }

  FDKwriteBits( asc, getChannelConfig(config->channelMode), 4 );

  if (config->extAOT == AOT_SBR || config->extAOT == AOT_PS) {
    writeSampleRate(asc, config->extSamplingRate);
    writeAot(asc, config->aot);
  }

  switch (config->aot) {
#ifdef TP_GA_ENABLE
    case AOT_AAC_MAIN:
    case AOT_AAC_LC:
    case AOT_AAC_SSR:
    case AOT_AAC_LTP:
    case AOT_AAC_SCAL:
    case AOT_TWIN_VQ:
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LTP:
    case AOT_ER_AAC_SCAL:
    case AOT_ER_TWIN_VQ:
    case AOT_ER_BSAC:
    case AOT_ER_AAC_LD:
      err = transportEnc_writeGASpecificConfig(asc, config, extFlag, alignAnchor);
      if (err)
        return err;
      break;
#endif /* TP_GA_ENABLE */
#ifdef TP_ELD_ENABLE
    case AOT_ER_AAC_ELD:
      err = transportEnc_writeELDSpecificConfig(asc, config, epConfig, cb);
      if (err)
        return err;
      break;
#endif /* TP_ELD_ENABLE */
    default:
      return -1;
  }

  switch (config->aot) {
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LTP:
    case AOT_ER_AAC_SCAL:
    case AOT_ER_TWIN_VQ:
    case AOT_ER_BSAC:
    case AOT_ER_AAC_LD:
    case AOT_ER_CELP:
    case AOT_ER_HVXC:
    case AOT_ER_HILN:
    case AOT_ER_PARA:
    case AOT_ER_AAC_ELD:
      FDKwriteBits( asc, 0, 2 ); /* epconfig 0 */
      break;
    default:
      break;
  }

  /* Make sure all bits are sync'ed */
  FDKsyncCache( asc );

  return 0;
}
