
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2013 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
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

/*************************  Fast MPEG AAC Audio Encoder  **********************

   Initial author:       A. Groeschel
   contents/description: channel mapping functionality

******************************************************************************/

#include "channel_map.h"
#include "bitenc.h"
#include "psy_const.h"
#include "qc_data.h"
#include "aacEnc_ram.h"


/* channel_assignment treats the relationship of Input file channels
   to the encoder channels.
   This is necessary because the usual order in RIFF files (.wav)
   is different from the elements order in the coder given
   by Table 8.1 (implicit speaker mapping) of the AAC standard.

   In mono and stereo case, this is trivial.
   In mc case, it looks like this:

   Channel         Input file       coder chan
5ch:
   front center    2                0 (SCE channel)
   left center     0                1 (1st of 1st CPE)
   right center    1                2 (2nd of 1st CPE)
   left surround   3                3 (1st of 2nd CPE)
   right surround  4                4 (2nd of 2nd CPE)

5.1ch:
   front center    2                0 (SCE channel)
   left center     0                1 (1st of 1st CPE)
   right center    1                2 (2nd of 1st CPE)
   left surround   4                3 (1st of 2nd CPE)
   right surround  5                4 (2nd of 2nd CPE)
   LFE             3                5 (LFE)
*/

typedef struct {

  CHANNEL_MODE  encoderMode;
  INT           channel_assignment[/*(8)*/12];

} CHANNEL_ASSIGNMENT_INFO_TAB;


static const CHANNEL_ASSIGNMENT_INFO_TAB assignmentInfoTabMpeg[] =
{
     { MODE_INVALID,                 {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  invalid   */
     { MODE_1,                       { 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  mono      */
     { MODE_2,                       { 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  stereo    */
     { MODE_1_2,                     { 0, 1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  3ch       */
     { MODE_1_2_1,                   { 0, 1, 2, 3,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  4ch       */
     { MODE_1_2_2,                   { 0, 1, 2, 3, 4,-1,-1,-1,-1,-1,-1,-1} },  /*  5ch       */
     { MODE_1_2_2_1,                 { 0, 1, 2, 3, 4, 5,-1,-1,-1,-1,-1,-1} },  /*  5.1ch     */
     { MODE_1_2_2_2_1,               { 0, 1, 2, 3, 4, 5, 6, 7,-1,-1,-1,-1} },  /*  7.1ch     */
     { MODE_7_1_REAR_SURROUND,       { 0, 1, 2, 3, 4, 5, 6, 7,-1,-1,-1,-1} },  /*  7.1ch     */
     { MODE_7_1_FRONT_CENTER,        { 0, 1, 2, 3, 4, 5, 6, 7,-1,-1,-1,-1} }   /*  7.1ch     */
};

static const CHANNEL_ASSIGNMENT_INFO_TAB assignmentInfoTabWav[] =
{
     { MODE_INVALID,                 {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  invalid   */
     { MODE_1,                       { 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  mono      */
     { MODE_2,                       { 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  stereo    */
     { MODE_1_2,                     { 2, 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  3ch       */
     { MODE_1_2_1,                   { 2, 0, 1, 3,-1,-1,-1,-1,-1,-1,-1,-1} },  /*  4ch       */
     { MODE_1_2_2,                   { 2, 0, 1, 3, 4,-1,-1,-1,-1,-1,-1,-1} },  /*  5ch       */
     { MODE_1_2_2_1,                 { 2, 0, 1, 4, 5, 3,-1,-1,-1,-1,-1,-1} },  /*  5.1ch     */
     { MODE_1_2_2_2_1,               { 2, 6, 7, 0, 1, 4, 5, 3,-1,-1,-1,-1} },  /*  7.1ch     */
     { MODE_7_1_REAR_SURROUND,       { 2, 0, 1, 6, 7, 4, 5, 3,-1,-1,-1,-1} },  /*  7.1ch     */
     { MODE_7_1_FRONT_CENTER,        { 2, 6, 7, 0, 1, 4, 5, 3,-1,-1,-1,-1} },  /*  7.1ch     */
};

/* Channel mode configuration tab provides,
   corresponding number of channels and elements
*/
static const CHANNEL_MODE_CONFIG_TAB channelModeConfig[] =
{
   { MODE_1,                        1,  1,  1 },   /* SCE                 */
   { MODE_2,                        2,  2,  1 },   /* CPE                 */
   { MODE_1_2,                      3,  3,  2 },   /* SCE,CPE             */
   { MODE_1_2_1,                    4,  4,  3 },   /* SCE,CPE,SCE         */
   { MODE_1_2_2,                    5,  5,  3 },   /* SCE,CPE,CPE         */
   { MODE_1_2_2_1,                  6,  5,  4 },   /* SCE,CPE,CPE,LFE     */
   { MODE_1_2_2_2_1,                8,  7,  5 },   /* SCE,CPE,CPE,CPE,LFE */
   { MODE_7_1_REAR_SURROUND,        8,  7,  5 },
   { MODE_7_1_FRONT_CENTER,         8,  7,  5 },
};

#define MAX_MODES (sizeof(assignmentInfoTabWav)/sizeof(CHANNEL_ASSIGNMENT_INFO_TAB))

const INT* FDKaacEnc_getChannelAssignment(CHANNEL_MODE encMode, CHANNEL_ORDER co)
{
  const CHANNEL_ASSIGNMENT_INFO_TAB *pTab;
  int i;

  if (co == CH_ORDER_MPEG)
    pTab = assignmentInfoTabMpeg;
  else
    pTab = assignmentInfoTabWav;

  for(i=MAX_MODES-1; i>0; i--) {
    if (encMode== pTab[i].encoderMode) {
      break;
    }
  }
  return (pTab[i].channel_assignment);
}

AAC_ENCODER_ERROR FDKaacEnc_DetermineEncoderMode(CHANNEL_MODE* mode, INT nChannels)
{
  INT i;
  CHANNEL_MODE encMode = MODE_INVALID;

  if (*mode==MODE_UNKNOWN) {
    for (i=0; i<(INT)sizeof(channelModeConfig)/(INT)sizeof(CHANNEL_MODE_CONFIG_TAB); i++) {
      if (channelModeConfig[i].nChannels==nChannels) {
          encMode = channelModeConfig[i].encMode;
          break;
      }
    }
    *mode = encMode;
  }
  else {
    /* check if valid channel configuration */
    if (FDKaacEnc_GetChannelModeConfiguration(*mode)->nChannels==nChannels) {
        encMode = *mode;
    }
  }

  if (encMode==MODE_INVALID) {
    return AAC_ENC_UNSUPPORTED_CHANNELCONFIG;
  }

  return AAC_ENC_OK;
}

static INT FDKaacEnc_initElement (ELEMENT_INFO* elInfo, MP4_ELEMENT_ID elType, INT* cnt, CHANNEL_MODE mode, CHANNEL_ORDER co, INT* it_cnt, const FIXP_DBL relBits) {

  INT error=0;
  INT counter =*cnt;

  const INT *assign = FDKaacEnc_getChannelAssignment(mode, co);

  elInfo->elType=elType;
  elInfo->relativeBits = relBits;

  switch(elInfo->elType) {
  case ID_SCE: case ID_LFE: case ID_CCE:
    elInfo->nChannelsInEl=1;
    elInfo->ChannelIndex[0]=assign[counter++];
    elInfo->instanceTag=it_cnt[elType]++;

    break;
  case ID_CPE:
    elInfo->nChannelsInEl=2;
    elInfo->ChannelIndex[0]=assign[counter++];
    elInfo->ChannelIndex[1]=assign[counter++];
    elInfo->instanceTag=it_cnt[elType]++;
    break;
  case ID_DSE:
    elInfo->nChannelsInEl=0;
    elInfo->ChannelIndex[0]=0;
    elInfo->ChannelIndex[1]=0;
    elInfo->instanceTag=it_cnt[elType]++;
    break;
  default: error=1;
  };
  *cnt = counter;
  return error;

}

AAC_ENCODER_ERROR FDKaacEnc_InitChannelMapping(CHANNEL_MODE mode, CHANNEL_ORDER co, CHANNEL_MAPPING* cm)
{
  INT count=0; /* count through coder channels */
  INT it_cnt[ID_END+1];
  INT i;

  for (i=0; i<ID_END; i++)
    it_cnt[i]=0;

  FDKmemclear(cm, sizeof(CHANNEL_MAPPING));

  /* init channel mapping*/
  for (i=0; i<(INT)sizeof(channelModeConfig)/(INT)sizeof(CHANNEL_MODE_CONFIG_TAB); i++) {
    if (channelModeConfig[i].encMode==mode)
    {
        cm->encMode      = channelModeConfig[i].encMode;
        cm->nChannels    = channelModeConfig[i].nChannels;
        cm->nChannelsEff = channelModeConfig[i].nChannelsEff;
        cm->nElements    = channelModeConfig[i].nElements;

        break;
    }
  }

  /* init element info struct */
  switch(mode) {
    case MODE_1:
      /* (mono) sce  */
      FDKaacEnc_initElement(&cm->elInfo[0], ID_SCE, &count, mode, co, it_cnt, (FIXP_DBL)MAXVAL_DBL);
      break;
    case MODE_2:
      /* (stereo) cpe */
      FDKaacEnc_initElement(&cm->elInfo[0], ID_CPE, &count, mode, co, it_cnt, (FIXP_DBL)MAXVAL_DBL);
      break;

    case MODE_1_2:
      /* sce + cpe */
      FDKaacEnc_initElement(&cm->elInfo[0], ID_SCE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.4f));
      FDKaacEnc_initElement(&cm->elInfo[1], ID_CPE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.6f));
      break;

    case MODE_1_2_1:
      /* sce + cpe + sce */
      FDKaacEnc_initElement(&cm->elInfo[0], ID_SCE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.3f));
      FDKaacEnc_initElement(&cm->elInfo[1], ID_CPE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.4f));
      FDKaacEnc_initElement(&cm->elInfo[2], ID_SCE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.3f));
      break;

    case MODE_1_2_2:
      /* sce + cpe + cpe */
      FDKaacEnc_initElement(&cm->elInfo[0], ID_SCE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.26f));
      FDKaacEnc_initElement(&cm->elInfo[1], ID_CPE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.37f));
      FDKaacEnc_initElement(&cm->elInfo[2], ID_CPE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.37f));
      break;

    case MODE_1_2_2_1:
      /* (5.1) sce + cpe + cpe + lfe */
      FDKaacEnc_initElement(&cm->elInfo[0], ID_SCE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.24f));
      FDKaacEnc_initElement(&cm->elInfo[1], ID_CPE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.35f));
      FDKaacEnc_initElement(&cm->elInfo[2], ID_CPE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.35f));
      FDKaacEnc_initElement(&cm->elInfo[3], ID_LFE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.06f));
      break;

    case MODE_1_2_2_2_1:
    case MODE_7_1_REAR_SURROUND:
    case MODE_7_1_FRONT_CENTER:
      /* (7.1) sce + cpe + cpe + cpe + lfe */
      FDKaacEnc_initElement(&cm->elInfo[0], ID_SCE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.18f));
      FDKaacEnc_initElement(&cm->elInfo[1], ID_CPE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.26f));
      FDKaacEnc_initElement(&cm->elInfo[2], ID_CPE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.26f));
      FDKaacEnc_initElement(&cm->elInfo[3], ID_CPE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.26f));
      FDKaacEnc_initElement(&cm->elInfo[4], ID_LFE, &count, mode, co, it_cnt, FL2FXCONST_DBL(0.04f));
      break;
    default:
      //*chMap=0;
      return AAC_ENC_UNSUPPORTED_CHANNELCONFIG;
  };


  FDK_ASSERT(cm->nElements<=(8));


  return AAC_ENC_OK;
}

AAC_ENCODER_ERROR FDKaacEnc_InitElementBits(QC_STATE *hQC,
                    CHANNEL_MAPPING *cm,
                    INT bitrateTot,
                    INT averageBitsTot,
                    INT maxChannelBits)
{
  int sc_brTot = CountLeadingBits(bitrateTot);

  switch(cm->encMode) {
  case MODE_1:
    hQC->elementBits[0]->chBitrateEl      = bitrateTot;

    hQC->elementBits[0]->maxBitsEl        = maxChannelBits;

    hQC->elementBits[0]->relativeBitsEl   = cm->elInfo[0].relativeBits;
    break;

  case MODE_2:
    hQC->elementBits[0]->chBitrateEl      = bitrateTot>>1;

    hQC->elementBits[0]->maxBitsEl        = 2*maxChannelBits;

    hQC->elementBits[0]->relativeBitsEl   = cm->elInfo[0].relativeBits;
    break;
  case MODE_1_2: {
    hQC->elementBits[0]->relativeBitsEl = cm->elInfo[0].relativeBits;
    hQC->elementBits[1]->relativeBitsEl = cm->elInfo[1].relativeBits;
    FIXP_DBL sceRate = cm->elInfo[0].relativeBits;
    FIXP_DBL cpeRate = cm->elInfo[1].relativeBits;

    hQC->elementBits[0]->chBitrateEl      = fMult(sceRate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>sc_brTot;
    hQC->elementBits[1]->chBitrateEl      = fMult(cpeRate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>(sc_brTot+1);

    hQC->elementBits[0]->maxBitsEl        = maxChannelBits;
    hQC->elementBits[1]->maxBitsEl        = 2*maxChannelBits;
    break;
  }
  case MODE_1_2_1: {
    /* sce + cpe + sce */
    hQC->elementBits[0]->relativeBitsEl = cm->elInfo[0].relativeBits;
    hQC->elementBits[1]->relativeBitsEl = cm->elInfo[1].relativeBits;
    hQC->elementBits[2]->relativeBitsEl = cm->elInfo[2].relativeBits;
    FIXP_DBL sce1Rate = cm->elInfo[0].relativeBits;
    FIXP_DBL cpeRate  = cm->elInfo[1].relativeBits;
    FIXP_DBL sce2Rate = cm->elInfo[2].relativeBits;

    hQC->elementBits[0]->chBitrateEl      = fMult(sce1Rate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>sc_brTot;
    hQC->elementBits[1]->chBitrateEl      = fMult(cpeRate,  (FIXP_DBL)(bitrateTot<<sc_brTot))>>(sc_brTot+1);
    hQC->elementBits[2]->chBitrateEl      = fMult(sce2Rate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>sc_brTot;

    hQC->elementBits[0]->maxBitsEl        = maxChannelBits;
    hQC->elementBits[1]->maxBitsEl        = 2*maxChannelBits;
    hQC->elementBits[2]->maxBitsEl        = maxChannelBits;
    break;
  }
  case MODE_1_2_2: {
    /* sce + cpe + cpe */
    hQC->elementBits[0]->relativeBitsEl = cm->elInfo[0].relativeBits;
    hQC->elementBits[1]->relativeBitsEl = cm->elInfo[1].relativeBits;
    hQC->elementBits[2]->relativeBitsEl = cm->elInfo[2].relativeBits;
    FIXP_DBL sceRate  = cm->elInfo[0].relativeBits;
    FIXP_DBL cpe1Rate = cm->elInfo[1].relativeBits;
    FIXP_DBL cpe2Rate = cm->elInfo[2].relativeBits;

    hQC->elementBits[0]->chBitrateEl      = fMult(sceRate,  (FIXP_DBL)(bitrateTot<<sc_brTot))>>sc_brTot;
    hQC->elementBits[1]->chBitrateEl      = fMult(cpe1Rate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>(sc_brTot+1);
    hQC->elementBits[2]->chBitrateEl      = fMult(cpe2Rate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>(sc_brTot+1);

    hQC->elementBits[0]->maxBitsEl        = maxChannelBits;
    hQC->elementBits[1]->maxBitsEl        = 2*maxChannelBits;
    hQC->elementBits[2]->maxBitsEl        = 2*maxChannelBits;
    break;
  }

  case MODE_1_2_2_1: {
    /* (5.1) sce + cpe + cpe + lfe */
    hQC->elementBits[0]->relativeBitsEl = cm->elInfo[0].relativeBits;
    hQC->elementBits[1]->relativeBitsEl = cm->elInfo[1].relativeBits;
    hQC->elementBits[2]->relativeBitsEl = cm->elInfo[2].relativeBits;
    hQC->elementBits[3]->relativeBitsEl = cm->elInfo[3].relativeBits;
    FIXP_DBL sceRate  = cm->elInfo[0].relativeBits;
    FIXP_DBL cpe1Rate = cm->elInfo[1].relativeBits;
    FIXP_DBL cpe2Rate = cm->elInfo[2].relativeBits;
    FIXP_DBL lfeRate  = cm->elInfo[3].relativeBits;

    int maxBitsTot = maxChannelBits * 5; /* LFE does not add to bit reservoir */
    int sc = CountLeadingBits(fixMax(maxChannelBits,averageBitsTot));
    int maxLfeBits = (int) FDKmax ( (INT)((fMult(lfeRate,(FIXP_DBL)(maxChannelBits<<sc))>>sc)<<1),
                                    (INT)((fMult(FL2FXCONST_DBL(1.1f/2.f),fMult(lfeRate,(FIXP_DBL)(averageBitsTot<<sc)))<<1)>>sc) );

    maxChannelBits = (maxBitsTot - maxLfeBits);
    sc = CountLeadingBits(maxChannelBits);

    maxChannelBits = fMult((FIXP_DBL)maxChannelBits<<sc,GetInvInt(5))>>sc;

    hQC->elementBits[0]->chBitrateEl      = fMult(sceRate,  (FIXP_DBL)(bitrateTot<<sc_brTot))>>sc_brTot;
    hQC->elementBits[1]->chBitrateEl      = fMult(cpe1Rate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>(sc_brTot+1);
    hQC->elementBits[2]->chBitrateEl      = fMult(cpe2Rate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>(sc_brTot+1);
    hQC->elementBits[3]->chBitrateEl      = fMult(lfeRate,  (FIXP_DBL)(bitrateTot<<sc_brTot))>>sc_brTot;

    hQC->elementBits[0]->maxBitsEl        = maxChannelBits;
    hQC->elementBits[1]->maxBitsEl        = 2*maxChannelBits;
    hQC->elementBits[2]->maxBitsEl        = 2*maxChannelBits;
    hQC->elementBits[3]->maxBitsEl        = maxLfeBits;

    break;
  }
  case MODE_7_1_REAR_SURROUND:
  case MODE_7_1_FRONT_CENTER:
  case MODE_1_2_2_2_1: {
    int cpe3Idx = 3;
    int lfeIdx  = 4;

    /* (7.1) sce + cpe + cpe + cpe + lfe */
    FIXP_DBL sceRate  = hQC->elementBits[0]->relativeBitsEl = cm->elInfo[0].relativeBits;
    FIXP_DBL cpe1Rate = hQC->elementBits[1]->relativeBitsEl = cm->elInfo[1].relativeBits;
    FIXP_DBL cpe2Rate = hQC->elementBits[2]->relativeBitsEl = cm->elInfo[2].relativeBits;
    FIXP_DBL cpe3Rate = hQC->elementBits[cpe3Idx]->relativeBitsEl = cm->elInfo[cpe3Idx].relativeBits;
    FIXP_DBL lfeRate  = hQC->elementBits[lfeIdx]->relativeBitsEl = cm->elInfo[lfeIdx].relativeBits;

    int maxBitsTot = maxChannelBits * 7; /* LFE does not add to bit reservoir */
    int sc = CountLeadingBits(fixMax(maxChannelBits,averageBitsTot));
    int maxLfeBits = (int) FDKmax ( (INT)((fMult(lfeRate,(FIXP_DBL)(maxChannelBits<<sc))>>sc)<<1),
                                    (INT)((fMult(FL2FXCONST_DBL(1.1f/2.f),fMult(lfeRate,(FIXP_DBL)(averageBitsTot<<sc)))<<1)>>sc) );

    maxChannelBits = (maxBitsTot - maxLfeBits) / 7;

    hQC->elementBits[0]->chBitrateEl = fMult(sceRate,  (FIXP_DBL)(bitrateTot<<sc_brTot))>>sc_brTot;
    hQC->elementBits[1]->chBitrateEl = fMult(cpe1Rate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>(sc_brTot+1);
    hQC->elementBits[2]->chBitrateEl = fMult(cpe2Rate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>(sc_brTot+1);
    hQC->elementBits[cpe3Idx]->chBitrateEl = fMult(cpe3Rate, (FIXP_DBL)(bitrateTot<<sc_brTot))>>(sc_brTot+1);
    hQC->elementBits[lfeIdx]->chBitrateEl = fMult(lfeRate,  (FIXP_DBL)(bitrateTot<<sc_brTot))>>sc_brTot;

    hQC->elementBits[0]->maxBitsEl = maxChannelBits;
    hQC->elementBits[1]->maxBitsEl = 2*maxChannelBits;
    hQC->elementBits[2]->maxBitsEl = 2*maxChannelBits;
    hQC->elementBits[cpe3Idx]->maxBitsEl = 2*maxChannelBits;
    hQC->elementBits[lfeIdx]->maxBitsEl = maxLfeBits;
    break;
  }
  default:
    return AAC_ENC_UNSUPPORTED_CHANNELCONFIG;
  }

  return AAC_ENC_OK;
}

/********************************************************************************/
/*                                                                              */
/* function:    GetMonoStereoMODE(const CHANNEL_MODE mode)                      */
/*                                                                              */
/* description: Determines encoder setting from channel mode.                   */
/*              Multichannel modes are mapped to mono or stereo modes           */
/*              returns MODE_MONO in case of mono,                           */
/*                      MODE_STEREO in case of stereo                        */
/*                      MODE_INVALID in case of error                        */
/*                                                                              */
/* input:       CHANNEL_MODE mode: Encoder mode (see qc_data.h).                */
/* output:      return: CM_STEREO_MODE monoStereoSetting                        */
/*              (MODE_INVALID: error,                                        */
/*               MODE_MONO:    mono                                          */
/*               MODE_STEREO:  stereo).                                      */
/*                                                                              */
/* misc:        No memory is allocated.                                         */
/*                                                                              */
/********************************************************************************/

ELEMENT_MODE FDKaacEnc_GetMonoStereoMode(const CHANNEL_MODE mode){

  ELEMENT_MODE monoStereoSetting = EL_MODE_INVALID;

  switch(mode){
  case MODE_1:        /* mono setups */
    monoStereoSetting = EL_MODE_MONO;
    break;
  case MODE_2:        /* stereo setups */
  case MODE_1_2:
  case MODE_1_2_1:
  case MODE_1_2_2:
  case MODE_1_2_2_1:
  case MODE_1_2_2_2_1:
  case MODE_7_1_REAR_SURROUND:
  case MODE_7_1_FRONT_CENTER:
    monoStereoSetting = EL_MODE_STEREO;
    break;
  default:            /* error */
    monoStereoSetting = EL_MODE_INVALID;
    break;
  }

  return monoStereoSetting;
}

const CHANNEL_MODE_CONFIG_TAB* FDKaacEnc_GetChannelModeConfiguration(const CHANNEL_MODE mode)
{
  INT i;
  const CHANNEL_MODE_CONFIG_TAB *cm_config = NULL;

  /* get channel mode config */
  for (i=0; i<(INT)sizeof(channelModeConfig)/(INT)sizeof(CHANNEL_MODE_CONFIG_TAB); i++) {
    if (channelModeConfig[i].encMode==mode)
    {
        cm_config = &channelModeConfig[i];
        break;
    }
  }
  return cm_config;
}
