
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

/****************************  FDK PCM utils module  **************************

   Author(s):   Christian Griebel
   Description: Defines functions to interface with the PCM post processing
                module.

*******************************************************************************/

#include "pcmutils_lib.h"

#include "genericStds.h"
#include "fixpoint_math.h"

/* Decoder library info */
#define PCMDMX_LIB_VL0 2
#define PCMDMX_LIB_VL1 4
#define PCMDMX_LIB_VL2 0
#define PCMDMX_LIB_TITLE "PCM Downmix Lib"
#define PCMDMX_LIB_BUILD_DATE __DATE__
#define PCMDMX_LIB_BUILD_TIME __TIME__

/* Library settings */
#define PCM_DMX_MAX_DELAY_FRAMES        ( 1 )
#define PCM_DMX_MAX_CHANNELS            ( 8 )
#define PCM_DMX_MAX_CHANNEL_GROUPS      ( 4 )
#define PCM_DMX_MAX_CHANNELS_PER_GROUP  ( 3 )   /* The maximum over all groups */
#define PCMDMX_DFLT_EXPIRY_FRAME        ( 50 )  /* At least 500ms (FL 960 @ 96kHz) */

/* Fixed and unique channel group indices.
 * The last group index has to be smaller than PCM_DMX_MAX_CHANNEL_GROUPS. */
#define CH_GROUP_FRONT ( 0 )
#define CH_GROUP_SIDE  ( 1 )
#define CH_GROUP_REAR  ( 2 )
#define CH_GROUP_LFE   ( 3 )

/* The ordering of the following fixed channel labels has to be in MPEG-4 style.
 * From the center to the back with left and right channel interleaved (starting with left).
 * The last channel label index has to be smaller than PCM_DMX_MAX_CHANNELS. */
#define CENTER_FRONT_CHANNEL    ( 0 )     /* C  */
#define LEFT_FRONT_CHANNEL      ( 1 )     /* L  */
#define RIGHT_FRONT_CHANNEL     ( 2 )     /* R  */
#define LEFT_OUTSIDE_CHANNEL    ( 3 )     /* Lo */
#define RIGHT_OUTSIDE_CHANNEL   ( 4 )     /* Ro */
#define LEFT_REAR_CHANNEL       ( 5 )     /* Lr  aka left back channel  */
#define RIGHT_REAR_CHANNEL      ( 6 )     /* Rr  aka right back channel */
#define LOW_FREQUENCY_CHANNEL   ( 7 )     /* Lf */

/* More constants */
#define ANC_DATA_SYNC_BYTE      ( 0xBC )  /* ancillary data sync byte. */
#define ATTENUATION_FACTOR_1    ( FL2FXCONST_SGL(0.70710678f) )
#define TWO_CHANNEL             ( 2 )

/* Sanity checks on library setting: */

/* List of packed channel modes */
typedef enum
{ /* CH_MODE_<numFrontCh>_<numOutsideCh>_<numRearCh>_<numLfCh> */
  CH_MODE_UNDEFINED = 0x0000,
  /* 1 channel */
  CH_MODE_1_0_0_0   = 0x0001,   /* chCfg 1 */
  /* 2 channels */
  CH_MODE_2_0_0_0   = 0x0002,   /* chCfg 2 */
  /* 3 channels */
  CH_MODE_3_0_0_0   = 0x0003,   /* chCfg 3 */
  CH_MODE_2_0_1_0   = 0x0102,
  CH_MODE_2_0_0_1   = 0x1002,
  /* 4 channels */
  CH_MODE_3_0_1_0   = 0x0103,   /* chCfg 4 */
  CH_MODE_2_0_2_0   = 0x0202,
  CH_MODE_2_0_1_1   = 0x1102,
  /* 5 channels */
  CH_MODE_3_0_2_0   = 0x0203,   /* chCfg 5 */
  CH_MODE_2_0_2_1   = 0x1202,
  CH_MODE_3_0_1_1   = 0x1103,
  CH_MODE_3_2_0_0   = 0x0023,
  /* 6 channels */
  CH_MODE_3_0_2_1   = 0x1203,   /* chCfg 6 */
  CH_MODE_3_2_1_0   = 0x0123,
  /* 7 channels */
  CH_MODE_2_2_2_1   = 0x1222,
  CH_MODE_3_2_1_1   = 0x1123,
  CH_MODE_3_2_2_0   = 0x0223,
  /* 8 channels */
  CH_MODE_3_2_2_1   = 0x1222,   /* chCfg 7 */
  CH_MODE_3_2_1_2   = 0x2123,
  CH_MODE_2_2_2_2   = 0x2222

} PCM_DMX_CHANNEL_MODE;


/* These are the channel configurations linked to
   the number of output channels give by the user: */
static const PCM_DMX_CHANNEL_MODE outChModeTable[PCM_DMX_MAX_CHANNELS] =
{
  CH_MODE_1_0_0_0,  /* 1 channel  */
  CH_MODE_2_0_0_0,  /* 2 channels */
  CH_MODE_3_0_0_0,  /* 3 channels */
  CH_MODE_3_0_1_0,  /* 4 channels */
  CH_MODE_3_0_2_0,  /* 5 channels */
  CH_MODE_3_0_2_1,  /* 6 channels */
  CH_MODE_3_2_2_0,  /* 7 channels */
  CH_MODE_3_2_2_1   /* 8 channels */
};

static const FIXP_SGL dvbDownmixFactors[8] =
{
  FL2FXCONST_SGL(1.0f),
  FL2FXCONST_SGL(0.841f),
  FL2FXCONST_SGL(0.707f),
  FL2FXCONST_SGL(0.596f),
  FL2FXCONST_SGL(0.500f),
  FL2FXCONST_SGL(0.422f),
  FL2FXCONST_SGL(0.355f),
  FL2FXCONST_SGL(0.0f)
};


  /* MPEG matrix mixdown:
      Set 1:  L' = (1 + 2^-0.5 + A )^-1 * [L + C * 2^-0.5 + A * Ls];
              R' = (1 + 2^-0.5 + A )^-1 * [R + C * 2^-0.5 + A * Rs];

      Set 2:  L' = (1 + 2^-0.5 + 2A )^-1 * [L + C * 2^-0.5 - A * (Ls + Rs)];
              R' = (1 + 2^-0.5 + 2A )^-1 * [R + C * 2^-0.5 + A * (Ls + Rs)];

      M = (3 + 2A)^-1 * [L + C + R + A*(Ls + Rs)];
  */
  static const FIXP_SGL mpegMixDownIdx2Coef[4] =
  {
    FL2FXCONST_SGL(0.70710678f),
    FL2FXCONST_SGL(0.5f),
    FL2FXCONST_SGL(0.35355339f),
    FL2FXCONST_SGL(0.0f)
  };

  static const FIXP_SGL mpegMixDownIdx2PreFact[4] =
  {
    FL2FXCONST_SGL(0.4142135623730950f),
    FL2FXCONST_SGL(0.4530818393219728f),
    FL2FXCONST_SGL(0.4852813742385703f),
    FL2FXCONST_SGL(0.5857864376269050f)
  };

  typedef struct
  {
    USHORT  matrixMixdownIdx;       /*!< MPEG mixdown index extracted from PCE.            */
    USHORT  pseudoSurroundEnable;   /*!< Pseudo surround enable flag extracted from PCE.   */
    USHORT  mixdownAvailable;       /*!< Will be set to 1 if we found a valid coefficient. */

  } MPEG_MIXDOWN_INFO;


typedef struct
{
  FIXP_SGL  centerMixLevelValue;    /*!< DVB mixdown level for the center channel extracted from anc data.  */
  FIXP_SGL  surroundMixLevelValue;  /*!< DVB mixdown level for back channels extracted from anc data.       */

  UCHAR     mixLevelsAvail;         /*!< Will be set to 1 if we found a valid coefficient.                  */

} DVB_MIXDOWN_LEVELS;


/* Modules main data structure: */
struct PCM_DMX_INSTANCE
{
  DVB_MIXDOWN_LEVELS  dvbMixDownLevels[PCM_DMX_MAX_DELAY_FRAMES+1];
  MPEG_MIXDOWN_INFO   mpegMixDownInfo[PCM_DMX_MAX_DELAY_FRAMES+1];
  DUAL_CHANNEL_MODE dualChannelMode;
  UINT expiryFrame;
  UINT expiryCount;
  SHORT numOutputChannels;
  UCHAR applyProcessing;
  UCHAR frameDelay;
};

/* Memory allocation macro */
C_ALLOC_MEM_STATIC(PcmDmxInstance, struct PCM_DMX_INSTANCE, 1)


/** Evaluate a given channel configuration and extract a packed channel mode and generate a channel offset table
 *  This function is the inverse to the getChannelDescription() routine.
 * @param [in] The total number of channels of the given configuration.
 * @param [in] Array holding the corresponding channel types for each channel.
 * @param [in] Array holding the corresponding channel type indices for each channel.
 * @param [out] Array where the buffer offsets for each channel are stored into.
 * @returns Returns the packed channel mode.
 **/
static
PCM_DMX_CHANNEL_MODE getChannelMode (
        const INT                numChannels,                           /* in */
        const AUDIO_CHANNEL_TYPE channelType[],                         /* in */
        const UCHAR              channelIndices[],                      /* in */
        UCHAR                    offsetTable[PCM_DMX_MAX_CHANNELS]      /* out */
      )
{
  UINT  chMode = CH_MODE_UNDEFINED;
  UCHAR chIdx[PCM_DMX_MAX_CHANNEL_GROUPS][PCM_DMX_MAX_CHANNELS_PER_GROUP];
  UCHAR numChInGrp[PCM_DMX_MAX_CHANNEL_GROUPS];
  int   ch, grpIdx, err = 0;

  FDK_ASSERT(channelType != NULL);
  FDK_ASSERT(channelIndices != NULL);
  FDK_ASSERT(offsetTable != NULL);

  /* For details see ISO/IEC 13818-7:2005(E), 8.5.3 Channel configuration */
  FDKmemclear(numChInGrp, PCM_DMX_MAX_CHANNEL_GROUPS*sizeof(UCHAR));
  FDKmemset(offsetTable, 255, PCM_DMX_MAX_CHANNELS*sizeof(UCHAR));
  FDKmemset(chIdx, 255, PCM_DMX_MAX_CHANNEL_GROUPS*PCM_DMX_MAX_CHANNELS_PER_GROUP*sizeof(UCHAR));

  /* Categorize channels */
  for (ch = 0; ch < numChannels; ch += 1) {
    int i = 0, j, chGrpIdx = channelIndices[ch];

    switch (channelType[ch]) {
    case ACT_FRONT:
    case ACT_FRONT_TOP:
      grpIdx = CH_GROUP_FRONT;
      break;
    case ACT_SIDE:
    case ACT_SIDE_TOP:
      grpIdx = CH_GROUP_SIDE;
      break;
    case ACT_BACK:
    case ACT_BACK_TOP:
      grpIdx = CH_GROUP_REAR;
      break;
    case ACT_LFE:
      grpIdx = CH_GROUP_LFE;
      break;
    default:
      err = -1;
      continue;
    }

    if (numChInGrp[grpIdx] < PCM_DMX_MAX_CHANNELS_PER_GROUP) {
      /* Sort channels by index */
      while ( (i < numChInGrp[grpIdx]) && (chGrpIdx > channelIndices[chIdx[grpIdx][i]]) ) {
        i += 1;
      }
      for (j = numChInGrp[grpIdx]; j > i; j -= 1) {
        chIdx[grpIdx][j] = chIdx[grpIdx][j-1];
      }
      chIdx[grpIdx][i] = ch;
      numChInGrp[grpIdx] += 1;
    }
  }

  /* Compose channel offset table */

  /* Non-symmetric channels */
  if (numChInGrp[CH_GROUP_FRONT] & 0x1) {
    /* Odd number of front channels -> we have a center channel.
       In MPEG-4 the center has the index 0. */
    offsetTable[CENTER_FRONT_CHANNEL] = chIdx[CH_GROUP_FRONT][0];
  }

  for (grpIdx = 0; grpIdx < PCM_DMX_MAX_CHANNEL_GROUPS; grpIdx += 1) {
    int chMapPos, maxChannels = 0;
    ch = 0;

    switch (grpIdx) {
    case CH_GROUP_FRONT:
      chMapPos = LEFT_FRONT_CHANNEL;
      maxChannels = 3;
      ch = numChInGrp[grpIdx] & 0x1;
      break;
    case CH_GROUP_SIDE:
      chMapPos = LEFT_OUTSIDE_CHANNEL;
      maxChannels = 2;
      break;
    case CH_GROUP_REAR:
      chMapPos = LEFT_REAR_CHANNEL;
      maxChannels = 2;
      break;
    case CH_GROUP_LFE:
      chMapPos = LOW_FREQUENCY_CHANNEL;
      maxChannels = 1;
      break;
    default:
      err = -1;
      continue;
    }

    for ( ; ch < numChInGrp[grpIdx]; ch += 1) {
      if (ch < maxChannels) {
        offsetTable[chMapPos] = chIdx[grpIdx][ch];
        chMapPos += 1;
      } else {
        err = -1;
      }
    }
  }

  if (err == 0) {
    /* Compose the channel mode */
    chMode = (numChInGrp[CH_GROUP_LFE]   & 0xF) << 12
           | (numChInGrp[CH_GROUP_REAR]  & 0xF) <<  8
           | (numChInGrp[CH_GROUP_SIDE]  & 0xF) <<  4
           | (numChInGrp[CH_GROUP_FRONT] & 0xF);
  }

  return (PCM_DMX_CHANNEL_MODE)chMode;
}


/** Generate a channel offset table and complete channel description for a given (packed) channel mode.
 *  This function is the inverse to the getChannelMode() routine.
 * @param [in] The total number of channels of the given configuration.
 * @param [in] Array containing the channel mapping to be used (From MPEG PCE ordering to whatever is required).
 * @param [out] Array where corresponding channel types for each channels are stored into.
 * @param [out] Array where corresponding channel type indices for each output channel are stored into.
 * @param [out] Array where the buffer offsets for each channel are stored into.
 * @returns None.
 **/
void getChannelDescription (
        const PCM_DMX_CHANNEL_MODE  chMode,                                 /* in */
        const UCHAR                 channelMapping[][PCM_DMX_MAX_CHANNELS], /* in */
        AUDIO_CHANNEL_TYPE          channelType[],                          /* out */
        UCHAR                       channelIndices[],                       /* out */
        UCHAR                       offsetTable[PCM_DMX_MAX_CHANNELS]       /* out */
      )
{
  const UCHAR *pChannelMap;
  int   grpIdx, ch = 0, numChannels = 0;
  UCHAR numChInGrp[PCM_DMX_MAX_CHANNEL_GROUPS];

  FDK_ASSERT(channelType != NULL);
  FDK_ASSERT(channelIndices != NULL);
  FDK_ASSERT(channelMapping != NULL);
  FDK_ASSERT(offsetTable != NULL);

  /* Init output arrays */
  FDKmemclear(channelType,    PCM_DMX_MAX_CHANNELS*sizeof(AUDIO_CHANNEL_TYPE));
  FDKmemclear(channelIndices, PCM_DMX_MAX_CHANNELS*sizeof(UCHAR));
  FDKmemset(offsetTable, 255, PCM_DMX_MAX_CHANNELS*sizeof(UCHAR));

  /* Extract the number of channels per group */
  numChInGrp[CH_GROUP_FRONT] =  chMode        & 0xF;
  numChInGrp[CH_GROUP_SIDE]  = (chMode >>  4) & 0xF;
  numChInGrp[CH_GROUP_REAR]  = (chMode >>  8) & 0xF;
  numChInGrp[CH_GROUP_LFE]   = (chMode >> 12) & 0xF;

  /* Summerize to get the total number of channels */
  for (grpIdx = 0; grpIdx < PCM_DMX_MAX_CHANNEL_GROUPS; grpIdx += 1) {
    numChannels += numChInGrp[grpIdx];
  }

  /* Get the appropriate channel map */
  pChannelMap = channelMapping[numChannels-1];

  /* Compose channel offset table */

  /* Non-symmetric channels */
  if (numChInGrp[CH_GROUP_FRONT] & 0x1) {
    /* Odd number of front channels -> we have a center channel.
       In MPEG-4 the center has the index 0. */
    offsetTable[CENTER_FRONT_CHANNEL] = pChannelMap[0];
    channelType[0] = ACT_FRONT;
    ch += 1;
  }

  for (grpIdx = 0; grpIdx < PCM_DMX_MAX_CHANNEL_GROUPS; grpIdx += 1) {
    AUDIO_CHANNEL_TYPE type;
    int chMapPos, maxChannels = 0;
    int chIdx = 0;

    switch (grpIdx) {
    case CH_GROUP_FRONT:
      type = ACT_FRONT;
      chMapPos = LEFT_FRONT_CHANNEL;
      maxChannels = 3;
      chIdx = numChInGrp[grpIdx] & 0x1;
      break;
    case CH_GROUP_SIDE:
      type = ACT_SIDE;
      chMapPos = LEFT_OUTSIDE_CHANNEL;
      maxChannels = 2;
      break;
    case CH_GROUP_REAR:
      type = ACT_BACK;
      chMapPos = LEFT_REAR_CHANNEL;
      maxChannels = 2;
      break;
    case CH_GROUP_LFE:
      type = ACT_LFE;
      chMapPos = LOW_FREQUENCY_CHANNEL;
      maxChannels = 1;
      break;
    default:
      break;
    }

    for ( ; (chIdx < numChInGrp[grpIdx]) && (chIdx < maxChannels); chIdx += 1) {
      offsetTable[chMapPos] = pChannelMap[ch];
      channelType[ch]    = type;
      channelIndices[ch] = chIdx;
      chMapPos += 1;
      ch += 1;
    }
  }
}


/** Open and initialize an instance of the PCM downmix module
 * @param [out] Pointer to a buffer receiving the handle of the new instance.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_Open (
    HANDLE_PCM_DOWNMIX *pSelf
  )
{
  HANDLE_PCM_DOWNMIX self;
  
  if (pSelf == NULL) {
    return (PCMDMX_INVALID_HANDLE);
  }

  *pSelf = NULL;

  self = (HANDLE_PCM_DOWNMIX) GetPcmDmxInstance( 0 );
  if (self == NULL) {
    return (PCMDMX_OUT_OF_MEMORY);
  }

  /* Reset the full instance */
  pcmDmx_Reset( self, PCMDMX_RESET_FULL );

  *pSelf = self;

  return (PCMDMX_OK);
}


/** Reset all static values like e.g. mixdown coefficients.
 * @param [in] Handle of PCM downmix module instance.
 * @param [in] Flags telling which parts of the module shall be reset.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_Reset (
    HANDLE_PCM_DOWNMIX  self,
    UINT                flags
  )
{
  if (self == NULL) { return (PCMDMX_INVALID_HANDLE); }

  if (flags & PCMDMX_RESET_PARAMS) {
    self->dualChannelMode   = STEREO_MODE;
    self->numOutputChannels = 0;
    self->applyProcessing   = 0;
    self->frameDelay        = 0;
    self->expiryFrame       = PCMDMX_DFLT_EXPIRY_FRAME;
  }

  if (flags & PCMDMX_RESET_BS_DATA) {
    int slot;
    for (slot = 0; slot <= PCM_DMX_MAX_DELAY_FRAMES; slot += 1) {
      self->dvbMixDownLevels[slot].centerMixLevelValue    = dvbDownmixFactors[2]; /* 0.707 */
      self->dvbMixDownLevels[slot].surroundMixLevelValue  = dvbDownmixFactors[0]; /* 1.000 */
      self->dvbMixDownLevels[slot].mixLevelsAvail = 0;

      self->mpegMixDownInfo[slot].mixdownAvailable = 0;
    }
    /* Reset expiry counter */
    self->expiryCount = 0;
  }

  return (PCMDMX_OK);
}


/** Set one parameter for one instance of the PCM downmix module.
 * @param [in] Handle of PCM downmix module instance.
 * @param [in] Parameter to be set.
 * @param [in] Parameter value.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_SetParam (
    HANDLE_PCM_DOWNMIX  self,
    PCMDMX_PARAM        param,
    UINT                value
  )
{
  switch (param)
  {
  case DMX_BS_DATA_EXPIRY_FRAME:
    if (self == NULL)
      return (PCMDMX_INVALID_HANDLE);
    self->expiryFrame = value;
    break;

  case DMX_BS_DATA_DELAY:
    if (value > PCM_DMX_MAX_DELAY_FRAMES) {
      return (PCMDMX_UNABLE_TO_SET_PARAM);
    }
    if (self == NULL) {
      return (PCMDMX_INVALID_HANDLE);
    }
    self->frameDelay = value;
    break;

  case NUMBER_OF_OUTPUT_CHANNELS:
    switch ((int)value) {  /* supported output channels */
    case -1: case 0: case 1: case 2:
    case 6: case 8:
      break;
    default:
      return (PCMDMX_UNABLE_TO_SET_PARAM);
    }
    if (self == NULL)
      return (PCMDMX_INVALID_HANDLE);
    if ((int)value > 0) {
      self->numOutputChannels = (int)value;
      self->applyProcessing = 1;
    } else {
      self->numOutputChannels = 0;
      self->applyProcessing = 0;
    }
    break;

  case DUAL_CHANNEL_DOWNMIX_MODE:
    switch ((DUAL_CHANNEL_MODE)value) {
    case STEREO_MODE:
    case CH1_MODE:
    case CH2_MODE:
    case MIXED_MODE:
      break;
    default:
      return (PCMDMX_UNABLE_TO_SET_PARAM);
    }
    if (self == NULL)
      return (PCMDMX_INVALID_HANDLE);
    self->dualChannelMode = (DUAL_CHANNEL_MODE)value;
    self->applyProcessing = 1;
    break;

  default:
    return (PCMDMX_UNKNOWN_PARAM);
  }

  return (PCMDMX_OK);
}


/** Read the ancillary data transported in DSEs of DVB streams with MPEG-4 content
 * @param [in] Handle of PCM downmix module instance.
 * @param [in] Pointer to ancillary data buffer.
 * @param [in] Size of ancillary data.
 * @param [in] Flag indicating wheter the DVB ancillary data is from an MPEG-1/2 or an MPEG-4 stream.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_ReadDvbAncData (
    HANDLE_PCM_DOWNMIX  self,
    UCHAR *pAncDataBuf,
    UINT   ancDataBytes,
    int    isMpeg2
  )
{
  DVB_MIXDOWN_LEVELS *pDownmixLevels = &self->dvbMixDownLevels[0];

  int   offset = (isMpeg2) ? 2 : 0;
  UCHAR ancDataStatus;

  if (self == NULL) { return (PCMDMX_INVALID_HANDLE); }

  /* sanity checks */
  if (pAncDataBuf == NULL || ancDataBytes < (UCHAR)(3+offset)) {
    return (PCMDMX_CORRUPT_ANC_DATA);
  }

  /* check sync word */
  if (pAncDataBuf[offset] != ANC_DATA_SYNC_BYTE) {
    return (PCMDMX_CORRUPT_ANC_DATA);
  }

  offset += 2;
  ancDataStatus = pAncDataBuf[offset++];

  if (isMpeg2) {
    /* skip advanced_dynamic_range_control */
    if (ancDataStatus & 0x80) offset += 3;
    /* skip dialog_normalization */
    if (ancDataStatus & 0x40) offset += 1;
    /* skip reproduction_level */
    if (ancDataStatus & 0x20) offset += 1;
  }
  else {
    /* check reserved bits */
    if (ancDataStatus & 0xE8) { return (PCMDMX_CORRUPT_ANC_DATA); }
  }

  /* downmix_levels_MPEGX */
  if (ancDataStatus & 0x10)
  {
    int   foundNewData = 0;
    UCHAR downmixData = pAncDataBuf[offset++];

    if (downmixData & 0x80) {  /* center_mix_level_on */
      pDownmixLevels->centerMixLevelValue =
        dvbDownmixFactors[(downmixData >> 4) & 0x07];
      foundNewData = 1;
    } else {
      pDownmixLevels->centerMixLevelValue = dvbDownmixFactors[0];
      if (downmixData & 0x70) { return (PCMDMX_CORRUPT_ANC_DATA); }
    }

    if (downmixData & 0x08) {  /* surround_mix_level_on */
      pDownmixLevels->surroundMixLevelValue =
        dvbDownmixFactors[downmixData & 0x07];
      foundNewData = 1;
    } else {
      pDownmixLevels->surroundMixLevelValue = dvbDownmixFactors[0];
      if (downmixData & 0x07) { return (PCMDMX_CORRUPT_ANC_DATA); }
    }

    pDownmixLevels->mixLevelsAvail = foundNewData;
  }

  /* Reset expiry counter */
  self->expiryCount = 0;

  return (PCMDMX_OK);
}

/** Set the matrix mixdown information extracted from the PCE of an AAC bitstream.
 *  Note: Call only if matrix_mixdown_idx_present is true.
 * @param [in] Handle of PCM downmix module instance.
 * @param [in] The 2 bit matrix mixdown index extracted from PCE.
 * @param [in] The pseudo surround enable flag extracted from PCE.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_SetMatrixMixdownFromPce (
    HANDLE_PCM_DOWNMIX  self,
    int                 matrixMixdownPresent,
    int                 matrixMixdownIdx,
    int                 pseudoSurroundEnable
  )
{
  MPEG_MIXDOWN_INFO *pMpegMixDownInfo;

  if (self == NULL) {
    return (PCMDMX_INVALID_HANDLE);
  }

  pMpegMixDownInfo = &self->mpegMixDownInfo[0];

  if (matrixMixdownPresent) {
    pMpegMixDownInfo->matrixMixdownIdx     = matrixMixdownIdx & 0x03;
    pMpegMixDownInfo->pseudoSurroundEnable = pseudoSurroundEnable;
  }

  pMpegMixDownInfo->mixdownAvailable = matrixMixdownPresent;
  /* Reset expiry counter */
  self->expiryCount = 0;

  return (PCMDMX_OK);
}


/** Apply down or up mixing.
 * @param [in]    Handle of PCM downmix module instance.
 * @param [inout] Pointer to time buffer. Depending on interface configuration, the content of pTimeData is ignored,
 *                and the internal QMF buffer will be used as input data source. Otherwise, the MPEG Surround processing is
 *                applied to the timesignal pTimeData. For both variants, the resulting MPEG Surround signal is written into pTimeData.
 * @param [in]    Pointer where the amount of output samples is returned into.
 * @param [inout] Pointer where the amount of output channels is returned into.
 * @param [in]    Flag which indicates if output time data are writtern interleaved or as subsequent blocks.
 * @param [inout] Array where the corresponding channel type for each output audio channel is stored into.
 * @param [inout] Array where the corresponding channel type index for each output audio channel is stored into.
 * @param [in]    Array containing the output channel mapping to be used (From MPEG PCE ordering to whatever is required).
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_ApplyFrame (
        HANDLE_PCM_DOWNMIX      self,
        INT_PCM                *pPcmBuf,
        UINT                    frameSize,
        INT                    *nChannels,

        int                     fInterleaved,
        AUDIO_CHANNEL_TYPE      channelType[],
        UCHAR                   channelIndices[],
        const UCHAR             channelMapping[][8]
  )
{
  PCMDMX_ERROR  errorStatus = PCMDMX_OK;
  DUAL_CHANNEL_MODE  dualChannelMode;
  PCM_DMX_CHANNEL_MODE  inChMode;
  int   numOutChannels;
  int   numInChannels = *nChannels;
  int   slot;
  UCHAR inOffsetTable[PCM_DMX_MAX_CHANNELS];

  MPEG_MIXDOWN_INFO   mpegMixDownInfo;
  DVB_MIXDOWN_LEVELS  dvbMixDownLevels;

  if (self == NULL) { return (PCMDMX_INVALID_HANDLE); }

  if ( (self->expiryFrame > 0)
    && (++self->expiryCount > self->expiryFrame) )
  { /* The metadata read from bitstream is too old. */
    errorStatus = pcmDmx_Reset(self, PCMDMX_RESET_BS_DATA);
  }

  FDKmemcpy(&mpegMixDownInfo, &self->mpegMixDownInfo[self->frameDelay], sizeof(MPEG_MIXDOWN_INFO));
  /* Maintain delay line */
  for (slot = self->frameDelay; slot > 0; slot -= 1) {
    FDKmemcpy(&self->mpegMixDownInfo[slot], &self->mpegMixDownInfo[slot-1], sizeof(MPEG_MIXDOWN_INFO));
  }
  FDKmemcpy(&dvbMixDownLevels, &self->dvbMixDownLevels[self->frameDelay], sizeof(DVB_MIXDOWN_LEVELS));
  /* Maintain delay line */
  for (slot = self->frameDelay; slot > 0; slot -= 1) {
    FDKmemcpy(&self->dvbMixDownLevels[slot], &self->dvbMixDownLevels[slot-1], sizeof(DVB_MIXDOWN_LEVELS));
  }

  if (self->applyProcessing == 0) { return (errorStatus); }

  if (pPcmBuf == NULL)     { return (PCMDMX_INVALID_ARGUMENT); }
  if (frameSize == 0)      { return (PCMDMX_INVALID_ARGUMENT); }
  if (numInChannels == 0)  { return (PCMDMX_INVALID_ARGUMENT); }

  if (self->numOutputChannels <= 0) {
    numOutChannels = numInChannels;
  } else {
    numOutChannels = self->numOutputChannels;
  }
  dualChannelMode = self->dualChannelMode;

  /* Analyse input channel configuration and get channel offset
   * table that can be accessed with the fixed channel labels. */
  inChMode = getChannelMode(
                   numInChannels,
                   channelType,
                   channelIndices,
                   inOffsetTable
                 );
  if (inChMode == CH_MODE_UNDEFINED) {
    /* We don't need to restore because the channel
       configuration has not been changed. Just exit. */
    return (PCMDMX_INVALID_CH_CONFIG);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  if ( numInChannels > numOutChannels )
  { /* Apply downmix */
    INT_PCM  *pInCF, *pInLF, *pInRF, *pInLO, *pInRO, *pInLR, *pInRR, *pOutL, *pOutR;
    FIXP_SGL  flev, clev, slev;

    UINT   sample;
    int    inStride, outStride, offset;
    int    useGuidedDownMix = 0;
    UCHAR  outOffsetTable[PCM_DMX_MAX_CHANNELS];

    /* Set I/O strides and offsets */
    if (fInterleaved) {
      inStride  = numInChannels;
      outStride = TWO_CHANNEL;   /* The output of STAGE ONE is always STEREO !!!
                                    STAGE TWO creates a downmix to mono if required. */
      offset = 1;                /* Channel specific offset factor */
    } else {
      inStride  = 1;
      outStride = 1;
      offset = frameSize;        /* Channel specific offset factor */
    }

    /* Get channel description and channel mapping for this
     * stages number of output channels (always STEREO). */
    getChannelDescription(
            CH_MODE_2_0_0_0,
            channelMapping,
            channelType,
            channelIndices,
            outOffsetTable
           );
    /* Now there is no way back because we modified the channel configuration! */

    /* Set channel pointer for input */
    pInCF = &pPcmBuf[inOffsetTable[CENTER_FRONT_CHANNEL]*offset];
    pInLF = &pPcmBuf[inOffsetTable[LEFT_FRONT_CHANNEL]*offset];
    pInRF = &pPcmBuf[inOffsetTable[RIGHT_FRONT_CHANNEL]*offset];
    pInLO = &pPcmBuf[inOffsetTable[LEFT_OUTSIDE_CHANNEL]*offset];
    pInRO = &pPcmBuf[inOffsetTable[RIGHT_OUTSIDE_CHANNEL]*offset];
    pInLR = &pPcmBuf[inOffsetTable[LEFT_REAR_CHANNEL]*offset];
    pInRR = &pPcmBuf[inOffsetTable[RIGHT_REAR_CHANNEL]*offset];

    /* Set channel pointer for output
       Caution: Different channel mapping compared to input */
    pOutL = &pPcmBuf[outOffsetTable[LEFT_FRONT_CHANNEL]*offset];    /* LEFT_FRONT_CHANNEL  */
    pOutR = &pPcmBuf[outOffsetTable[RIGHT_FRONT_CHANNEL]*offset];   /* RIGHT_FRONT_CHANNEL */

    /* Set downmix levels: */
    flev = ATTENUATION_FACTOR_1;    /* 0.707 */
    clev = ATTENUATION_FACTOR_1;    /* 0.707 */
    slev = ATTENUATION_FACTOR_1;    /* 0.707 */

    if ( dvbMixDownLevels.mixLevelsAvail ) {
      clev = dvbMixDownLevels.centerMixLevelValue;
      slev = dvbMixDownLevels.surroundMixLevelValue;
      useGuidedDownMix = 1;
    }

    /* FIRST STAGE:
         Always downmix to 2 channel output: */
    switch ( inChMode )
    {
    case CH_MODE_2_0_0_0:
    case CH_MODE_2_0_0_1: 
      /* 2/0 input: */
      switch (dualChannelMode)
      {
      case CH1_MODE:  /* L' = 0.707 * Ch1;  R' = 0.707 * Ch1 */
        for (sample = 0; sample < frameSize; sample++) {
          *pOutL = *pOutR =
            (INT_PCM)SATURATE_RIGHT_SHIFT(fMult((FIXP_PCM)*pInLF, flev), DFRACT_BITS-SAMPLE_BITS, SAMPLE_BITS);

          pInLF += inStride;
          pOutL += outStride; pOutR += outStride;
        }
        break;

      case CH2_MODE:  /* L' = 0.707 * Ch2;  R' = 0.707 * Ch2 */
        for (sample = 0; sample < frameSize; sample++) {
          *pOutL = *pOutR =
            (INT_PCM)SATURATE_RIGHT_SHIFT(fMult((FIXP_PCM)*pInRF, flev), DFRACT_BITS-SAMPLE_BITS, SAMPLE_BITS);

          pInRF += inStride;
          pOutL += outStride; pOutR += outStride;
        }
        break;
      case MIXED_MODE:  /* L' = 0.5*Ch1 + 0.5*Ch2;  R' = 0.5*Ch1 + 0.5*Ch2 */
        for (sample = 0; sample < frameSize; sample++) {
          *pOutL = *pOutR = (*pInLF >> 1) + (*pInRF >> 1);

          pInLF += inStride;  pInRF += inStride;
          pOutL += outStride; pOutR += outStride;
        }
        break;
      default:
      case STEREO_MODE:
        /* nothing to do */
        break;
      }
      break;

    case CH_MODE_3_0_0_0:
      /* 3/0 input:  L' = L + 0.707*C;  R' = R + 0.707*C; */
      for (sample = 0; sample < frameSize; sample++)
      {
        FIXP_DBL tCF = fMultDiv2((FIXP_PCM)*pInCF, clev);
#if (SAMPLE_BITS == 32)
        /* left channel */
        *pOutL = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>1)+tCF, 1, SAMPLE_BITS);
        /* right channel */
        *pOutR = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>1)+tCF, 1, SAMPLE_BITS);
#else
        /* left channel */
        *pOutL = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>1)+tCF, DFRACT_BITS-SAMPLE_BITS-1, SAMPLE_BITS);
        /* right channel */
        *pOutR = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>1)+tCF, DFRACT_BITS-SAMPLE_BITS-1, SAMPLE_BITS);
#endif
        pInLF += inStride;  pInRF += inStride;  pInCF  += inStride;
        pOutL += outStride; pOutR += outStride;
      }
      break;

    /* 2/1 input: not supported!
    case CH_MODE_2_0_1_0: */

    case CH_MODE_3_0_1_0:
      if (useGuidedDownMix) {
        /* 3/1 input:  L' = L + clev*C + 0.707*slev*S;  R' = R + clev*C + 0.707*slev*S; */
        slev = FX_DBL2FX_SGL(fMult(flev, slev));  /* 0.707*slef */

        for (sample = 0; sample < frameSize; sample++)
        {
          FIXP_DBL tCF = fMultDiv2((FIXP_PCM)*pInCF, clev) >> 1;
          FIXP_DBL tLR = fMultDiv2((FIXP_PCM)*pInLR, slev) >> 1;
#if (SAMPLE_BITS == 32)
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>2)+tCF+tLR, 2, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>2)+tCF+tLR, 2, SAMPLE_BITS);
#else
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>2)+tCF-tLR, DFRACT_BITS-SAMPLE_BITS-2, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>2)+tCF+tLR, DFRACT_BITS-SAMPLE_BITS-2, SAMPLE_BITS);
#endif
          pInLF += inStride;  pInRF += inStride;  pInCF  += inStride;  pInLR  += inStride;
          pOutL += outStride; pOutR += outStride;
        }
      } else {
        /* 3/1 input:  L' = L + 0.707*C - 0.707*S;  R' = R + 0.707*C + 0.707*S */
        for (sample = 0; sample < frameSize; sample++)
        {
          FIXP_DBL tCF = fMultDiv2((FIXP_PCM)*pInCF, clev) >> 1;
          FIXP_DBL tLR = fMultDiv2((FIXP_PCM)*pInLR, slev) >> 1;
#if (SAMPLE_BITS == 32)
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>2)+tCF-tLR, 2, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>2)+tCF+tLR, 2, SAMPLE_BITS);
#else
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>2)+tCF-tLR, DFRACT_BITS-SAMPLE_BITS-2, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>2)+tCF+tLR, DFRACT_BITS-SAMPLE_BITS-2, SAMPLE_BITS);
#endif
          pInLF += inStride;  pInRF += inStride;  pInCF  += inStride;  pInLR  += inStride;
          pOutL += outStride; pOutR += outStride;
        }
      }
      break;

    /* 2/2 input: not supported!
    case CH_MODE_2_0_2_0: */

    case CH_MODE_3_0_2_0:   /* 5.0ch input */
    case CH_MODE_3_0_2_1:   /* 5.1ch input */
      if (useGuidedDownMix) {
        /* 3/2 input:  L' = L + clev*C + slev*Ls;  R' = R + clev*C + slev*Rs; */
        for (sample = 0; sample < frameSize; sample++)
        {
          FIXP_DBL tCF = fMultDiv2((FIXP_PCM)*pInCF, clev) >> 1;
          FIXP_DBL tLR = fMultDiv2((FIXP_PCM)*pInLR, slev) >> 1;
          FIXP_DBL tRR = fMultDiv2((FIXP_PCM)*pInRR, slev) >> 1;
#if (SAMPLE_BITS == 32)
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>2)+tCF+tLR, 2, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>2)+tCF+tRR, 2, SAMPLE_BITS);
#else
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>2)+tCF+tLR, DFRACT_BITS-SAMPLE_BITS-2, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>2)+tCF+tRR, DFRACT_BITS-SAMPLE_BITS-2, SAMPLE_BITS);
#endif
          pInLF += inStride;  pInRF += inStride;  pInCF  += inStride;  pInLR  += inStride;  pInRR  += inStride;
          pOutL += outStride; pOutR += outStride;
        }
      }
      else if (mpegMixDownInfo.mixdownAvailable) {
        /* 3/2 input: L' = (1.707+A)^-1 * [L+0.707*C+A*Ls]; R'= (1.707+A)^-1 * [R+0.707*C+A*Rs]; */
        FIXP_SGL mtrxMixDwnCoef    = mpegMixDownIdx2Coef[mpegMixDownInfo.matrixMixdownIdx];
        FIXP_SGL mtrxMixDwnPreFact = mpegMixDownIdx2PreFact[mpegMixDownInfo.matrixMixdownIdx];
        clev = FX_DBL2FX_SGL(fMult(mtrxMixDwnPreFact, flev /* 0.707 */));
        flev = mtrxMixDwnPreFact;
        slev = FX_DBL2FX_SGL(fMult(mtrxMixDwnPreFact, mtrxMixDwnCoef));

        for (sample = 0; sample < frameSize; sample++)
        {
          FIXP_DBL tCF = fMultDiv2((FIXP_PCM)*pInCF, clev);
          FIXP_DBL tLF = fMultDiv2((FIXP_PCM)*pInLF, flev);
          FIXP_DBL tRF = fMultDiv2((FIXP_PCM)*pInRF, flev);
          FIXP_DBL tLR = fMultDiv2((FIXP_PCM)*pInLR, slev);
          FIXP_DBL tRR = fMultDiv2((FIXP_PCM)*pInRR, slev);

#if (SAMPLE_BITS == 32)
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_LEFT_SHIFT(tLF+tCF+tLR, 1, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_LEFT_SHIFT(tRF+tCF+tRR, 1, SAMPLE_BITS);
#else
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_RIGHT_SHIFT(tLF+tCF+tLR, DFRACT_BITS-SAMPLE_BITS-1, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_RIGHT_SHIFT(tRF+tCF+tRR, DFRACT_BITS-SAMPLE_BITS-1, SAMPLE_BITS);
#endif

          pInLF += inStride;  pInRF += inStride;  pInCF  += inStride;  pInLR  += inStride;  pInRR  += inStride;
          pOutL += outStride; pOutR += outStride;
        }
      }
      else {
        /* 3/2 input:  L' = L + 0.707*C - 0.707*Ls - 0.707*Rs;  R' = R + 0.707*C + 0.707*Ls + 0.707*Rs */
        for (sample = 0; sample < frameSize; sample++)
        {
          FIXP_DBL tCF = fMultDiv2((FIXP_PCM)*pInCF, clev) >> 2;
          FIXP_DBL tLR = fMultDiv2((FIXP_PCM)*pInLR, slev) >> 2;
          FIXP_DBL tRR = fMultDiv2((FIXP_PCM)*pInRR, slev) >> 2;
#if (SAMPLE_BITS == 32)
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>3)+tCF-tLR-tRR, 3, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_LEFT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>3)+tCF+tLR+tRR, 3, SAMPLE_BITS);
#else
          /* left channel */
          *pOutL = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInLF)>>3)+tCF-tLR-tRR, DFRACT_BITS-SAMPLE_BITS-3, SAMPLE_BITS);
          /* right channel */
          *pOutR = (INT_PCM)SATURATE_RIGHT_SHIFT((FX_PCM2FX_DBL((FIXP_PCM)*pInRF)>>3)+tCF+tLR+tRR, DFRACT_BITS-SAMPLE_BITS-3, SAMPLE_BITS);
#endif
          pInLF += inStride;  pInRF += inStride;  pInCF  += inStride;  pInLR  += inStride;  pInRR  += inStride;
          pOutL += outStride; pOutR += outStride;
        }
      }
      break;

    default:
      errorStatus = PCMDMX_INVALID_MODE;
      break;
    }

    /* SECOND STAGE:
         If desired create a mono donwmix:
         Note: Input are always two channels! */
    if (numOutChannels == 1)
    {
      INT_PCM *pOutC;
      FIXP_SGL mlev;

      if (useGuidedDownMix) mlev = FL2FXCONST_SGL(1.0f); else mlev = flev;

      /* Output of STAGE ONE = Input of STAGE TWO */
      FDKmemcpy(inOffsetTable, outOffsetTable, PCM_DMX_MAX_CHANNELS*sizeof(UCHAR));

      /* Set I/O strides and offsets */
      inStride  = outStride;          /* output from STAGE ONE */
      outStride = numOutChannels;     /* final output */

      /* Get channel description and channel mapping for this
       * stages number of output channels (always MONO). */
      getChannelDescription(
              CH_MODE_1_0_0_0,
              channelMapping,
              channelType,
              channelIndices,
              outOffsetTable
             );

      /* Set input channel pointer. */
      pInLF = &pPcmBuf[inOffsetTable[LEFT_FRONT_CHANNEL]*offset];
      pInRF = &pPcmBuf[inOffsetTable[RIGHT_FRONT_CHANNEL]*offset];

      /* Set output channel pointer */
      pOutC = &pPcmBuf[outOffsetTable[CENTER_FRONT_CHANNEL]*offset];

      /* C' = 0.707*L + 0.707*R */
      for (sample = 0; sample < frameSize; sample++) {
#if (SAMPLE_BITS == 32)
        *pOutC =
          (INT_PCM)SATURATE_LEFT_SHIFT(fMultDiv2((FIXP_PCM)*pInLF,mlev)+fMultDiv2((FIXP_PCM)*pInRF,mlev), 1, SAMPLE_BITS);
#else
        *pOutC =
          (INT_PCM)SATURATE_RIGHT_SHIFT(fMultDiv2((FIXP_PCM)*pInLF,mlev)+fMultDiv2((FIXP_PCM)*pInRF,mlev), DFRACT_BITS-SAMPLE_BITS-1, SAMPLE_BITS);
#endif

        pInLF += inStride; pInRF += inStride;
        pOutC += 1;
      }
      /* Finished STAGE TWO */
    }

    /* Update the number of output channels */
    *nChannels = self->numOutputChannels;

  } /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  else
  if ( numInChannels == numOutChannels )
  {
    /* Don't need to change the channel description here */

    switch (numInChannels)
    {
    case 2:
      { /* Set up channel pointer */
        INT_PCM *pInLF, *pInRF, *pOutL, *pOutR;
        FIXP_SGL flev;

        UINT sample;
        int inStride, outStride, offset;

        if (fInterleaved) {
          inStride  = numInChannels;
          outStride = 2;  /* fixed !!! (below stereo is donwmixed to mono if required */
          offset = 1; /* Channel specific offset factor */
        } else {
          inStride  = 1;
          outStride = 1;
          offset = frameSize;  /* Channel specific offset factor */
        }

        /* Set input channel pointer */
        pInLF = &pPcmBuf[inOffsetTable[LEFT_FRONT_CHANNEL]*offset];
        pInRF = &pPcmBuf[inOffsetTable[RIGHT_FRONT_CHANNEL]*offset];

        /* Set output channel pointer (same as input) */
        pOutL  =  pInLF;
        pOutR  =  pInRF;

        /* Set downmix levels: */
        flev = ATTENUATION_FACTOR_1;    /* 0.707 */
        /* 2/0 input: */
        switch (dualChannelMode)
        {
        case CH1_MODE:  /* L' = 0.707 * Ch1;  R' = 0.707 * Ch1 */
          for (sample = 0; sample < frameSize; sample++) {
            *pOutL = *pOutR =
              (INT_PCM)SATURATE_RIGHT_SHIFT(fMult((FIXP_PCM)*pInLF, flev), DFRACT_BITS-SAMPLE_BITS, SAMPLE_BITS);

            pInLF += inStride;
            pOutL += outStride; pOutR += outStride;
          }
          break;
        case CH2_MODE:  /* L' = 0.707 * Ch2;  R' = 0.707 * Ch2 */
          for (sample = 0; sample < frameSize; sample++) {
            *pOutL = *pOutR =
              (INT_PCM)SATURATE_RIGHT_SHIFT(fMult((FIXP_PCM)*pInRF, flev), DFRACT_BITS-SAMPLE_BITS, SAMPLE_BITS);

            pInRF += inStride;
            pOutL += outStride; pOutR += outStride;
          }
          break;
        case MIXED_MODE:  /* L' = 0.5*Ch1 + 0.5*Ch2;  R' = 0.5*Ch1 + 0.5*Ch2 */
          for (sample = 0; sample < frameSize; sample++) {
            *pOutL = *pOutR = (*pInLF >> 1) + (*pInRF >> 1);

            pInLF += inStride;  pInRF += inStride;
            pOutL += outStride; pOutR += outStride;
          }
          break;
        default:
        case STEREO_MODE:
          /* nothing to do */
          break;
        }
      }
      break;

    default:
      /* nothing to do */
      break;
    }
  }

  return (errorStatus);
}


/** Close an instance of the PCM downmix module.
 * @param [inout] Pointer to a buffer containing the handle of the instance.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_Close (
    HANDLE_PCM_DOWNMIX *pSelf
  )
{
  if (pSelf == NULL) {
    return (PCMDMX_INVALID_HANDLE);
  }

  FreePcmDmxInstance( pSelf );
  *pSelf = NULL;

  return (PCMDMX_OK);
}


/** Get library info for this module.
 * @param [out] Pointer to an allocated LIB_INFO structure.
 * @returns Returns an error code.
 */
PCMDMX_ERROR pcmDmx_GetLibInfo( LIB_INFO *info )
{
  int i;

  if (info == NULL) {
    return PCMDMX_INVALID_ARGUMENT;
  }

  /* Search for next free tab */
  for (i = 0; i < FDK_MODULE_LAST; i++) {
    if (info[i].module_id == FDK_NONE) break;
  }
  if (i == FDK_MODULE_LAST) {
    return PCMDMX_UNKNOWN;
  }
  info += i;

  /* Add the library info */
  info->module_id  = FDK_PCMDMX;
  info->version    = LIB_VERSION(PCMDMX_LIB_VL0, PCMDMX_LIB_VL1, PCMDMX_LIB_VL2);
  LIB_VERSION_STRING(info);
  info->build_date = PCMDMX_LIB_BUILD_DATE;
  info->build_time = PCMDMX_LIB_BUILD_TIME;
  info->title      = PCMDMX_LIB_TITLE;

  /* Set flags */
  info->flags = 0
      | CAPF_DMX_BLIND   /* At least blind downmixing is possible */
      | CAPF_DMX_PCE     /* Guided downmix with data from MPEG-2/4 Program Config Elements (PCE). */
      | CAPF_DMX_DVB     /* Guided downmix with data from DVB ancillary data fields. */
      ;

  return PCMDMX_OK;
}



