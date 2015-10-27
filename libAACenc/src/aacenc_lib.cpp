
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

/**************************** MPEG-4 HE-AAC Encoder *************************

  Initial author:       M. Lohwasser
  contents/description: FDK HE-AAC Encoder interface library functions

****************************************************************************/

#include "aacenc_lib.h"
#include "FDK_audio.h"
#include "aacenc.h"

#include "aacEnc_ram.h"
#include "FDK_core.h" /* FDK_tools versioning info */

/* Encoder library info */
#define AACENCODER_LIB_VL0 3
#define AACENCODER_LIB_VL1 4
#define AACENCODER_LIB_VL2 12
#define AACENCODER_LIB_TITLE "AAC Encoder"
#ifdef __ANDROID__
#define AACENCODER_LIB_BUILD_DATE ""
#define AACENCODER_LIB_BUILD_TIME ""
#else
#define AACENCODER_LIB_BUILD_DATE __DATE__
#define AACENCODER_LIB_BUILD_TIME __TIME__
#endif


#include "sbr_encoder.h"
#include "../src/sbr_ram.h"
#include "channel_map.h"

#include "psy_const.h"
#include "bitenc.h"

#include "tpenc_lib.h"

#include "metadata_main.h"

#define SBL(fl)            (fl/8)                 /*!< Short block length (hardcoded to 8 short blocks per long block) */
#define BSLA(fl)           (4*SBL(fl)+SBL(fl)/2)  /*!< AAC block switching look-ahead */
#define DELAY_AAC(fl)      (fl+BSLA(fl))          /*!< MDCT + blockswitching */
#define DELAY_AACELD(fl)   ((fl)/2)               /*!< ELD FB delay (no framing delay included) */

#define INPUTBUFFER_SIZE (1537+100+2048)

#define DEFAULT_HEADER_PERIOD_REPETITION_RATE  10 /*!< Default header repetition rate used in transport library and for SBR header. */

////////////////////////////////////////////////////////////////////////////////////
/**
 * Flags to characterize encoder modules to be supported in present instance.
 */
enum {
    ENC_MODE_FLAG_AAC  = 0x0001,
    ENC_MODE_FLAG_SBR  = 0x0002,
    ENC_MODE_FLAG_PS   = 0x0004,
    ENC_MODE_FLAG_SAC  = 0x0008,
    ENC_MODE_FLAG_META = 0x0010
};

////////////////////////////////////////////////////////////////////////////////////
typedef struct {
    AUDIO_OBJECT_TYPE userAOT;               /*!< Audio Object Type.             */
    UINT              userSamplerate;        /*!< Sampling frequency.            */
    UINT              nChannels;             /*!< will be set via channelMode.   */
    CHANNEL_MODE      userChannelMode;
    UINT              userBitrate;
    UINT              userBitrateMode;
    UINT              userBandwidth;
    UINT              userAfterburner;
    UINT              userFramelength;
    UINT              userAncDataRate;

    UCHAR             userTns;               /*!< Use TNS coding. */
    UCHAR             userPns;               /*!< Use PNS coding. */
    UCHAR             userIntensity;         /*!< Use Intensity coding. */

    TRANSPORT_TYPE    userTpType;            /*!< Transport type */
    UCHAR             userTpSignaling;       /*!< Extension AOT signaling mode. */
    UCHAR             userTpNsubFrames;      /*!< Number of sub frames in a transport frame for LOAS/LATM or ADTS (default 1). */
    UCHAR             userTpAmxv;            /*!< AudioMuxVersion to be used for LATM (default 0). */
    UCHAR             userTpProtection;
    UCHAR             userTpHeaderPeriod;    /*!< Parameter used to configure LATM/LOAS SMC rate. Moreover this parameters is
                                                  used to configure repetition rate of PCE in raw_data_block. */

    UCHAR             userErTools;           /*!< Use VCB11, HCR and/or RVLC ER tool. */
    UINT              userPceAdditions;      /*!< Configure additional bits in PCE. */

    UCHAR             userMetaDataMode;      /*!< Meta data library configuration. */

    UCHAR             userSbrEnabled;        /*!< Enable SBR for ELD. */
    UINT              userSbrRatio;          /*!< SBR sampling rate ratio. Dual- or single-rate. */

} USER_PARAM;

////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************
                           Structure Definitions
****************************************************************************/

typedef struct  AACENC_CONFIG     *HANDLE_AACENC_CONFIG;


struct AACENCODER
{
    USER_PARAM               extParam;
    CODER_CONFIG             coderConfig;

    /* AAC */
    AACENC_CONFIG            aacConfig;
    HANDLE_AAC_ENC           hAacEnc;

    /* SBR */
    HANDLE_SBR_ENCODER       hEnvEnc;

    /* Meta Data */
    HANDLE_FDK_METADATA_ENCODER  hMetadataEnc;
    INT                          metaDataAllowed; /* Signal whether chosen configuration allows metadata. Necessary for delay
                                                     compensation. Metadata mode is a separate parameter. */

    /* Transport */
    HANDLE_TRANSPORTENC      hTpEnc;

    /* Output */
    UCHAR                   *outBuffer;         /* Internal bitstream buffer */
    INT                      outBufferInBytes;   /* Size of internal bitstream buffer*/

    /* Input */
    INT_PCM                 *inputBuffer;        /* Internal input buffer. Input source for AAC encoder */
    INT                      inputBufferOffset;  /* Where to write new input samples. */

    INT                      nSamplesToRead;    /* number of input samples neeeded for encoding one frame */
    INT                      nSamplesRead;      /* number of input samples already in input buffer */
    INT                      nZerosAppended;    /* appended zeros at end of file*/
    INT                      nDelay;            /* encoder delay */

    AACENC_EXT_PAYLOAD       extPayload [MAX_TOTAL_EXT_PAYLOADS];
    /* Extension payload */
    UCHAR                    extPayloadData [(1)][(8)][MAX_PAYLOAD_SIZE];
    UINT                     extPayloadSize [(1)][(8)]; /* payload sizes in bits */

    ULONG                    InitFlags;         /* internal status to treggier re-initialization */


   /* Memory allocation info. */
   INT                       nMaxAacElements;
   INT                       nMaxAacChannels;
   INT                       nMaxSbrElements;
   INT                       nMaxSbrChannels;
   UINT                      nMaxSubFrames;

   UINT                      encoder_modis;

   /* Capability flags */
   UINT                      CAPF_tpEnc;

} ;

typedef struct
{
    ULONG               samplingRate;   /*!< Encoder output sampling rate. */
    ULONG               bitrateRange;   /*!< Lower bitrate range for config entry. */

    UCHAR               lowDelaySbr;    /*!< 0: ELD sbr off,
                                             1: ELD sbr on */

    UCHAR               downsampledSbr; /*!< 0: ELD with dualrate sbr,
                                             1: ELD with downsampled sbr */

} ELD_SBR_CONFIGURATOR;

/**
 * \brief  This table defines ELD/SBR default configurations.
 */
static const ELD_SBR_CONFIGURATOR eldSbrAutoConfigTab[] =
{
  { 48000,     0, 1, 0 },
  { 48000, 64001, 0, 0 },

  { 44100,     0, 1, 0 },
  { 44100, 64001, 0, 0 },

  { 32000,     0, 1, 0 },
  { 32000, 28000, 1, 1 },
  { 32000, 56000, 0, 0 },

  { 24000,     0, 1, 1 },
  { 24000, 40000, 0, 0 },

  { 16000,     0, 1, 1 },
  { 16000, 28000, 0, 0 }

};

/*
 * \brief  Configure SBR for ELD configuration.
 *
 * This function finds default SBR configuration for ELD based on sampling rate and channel bitrate.
 * Outputparameters are SBR on/off, and SBR ratio.
 *
 * \param samplingRate          Audio signal sampling rate.
 * \param channelMode           Channel configuration to be used.
 * \param totalBitrate          Overall bitrate.
 * \param eldSbr                Pointer to eldSbr parameter, filled on return.
 * \param eldSbrRatio           Pointer to eldSbrRatio parameter, filled on return.
 *
 * \return - AACENC_OK, all fine.
 *         - AACENC_INVALID_CONFIG, on failure.
 */
static AACENC_ERROR eldSbrConfigurator(
        const ULONG                      samplingRate,
        const CHANNEL_MODE               channelMode,
        const ULONG                      totalBitrate,
        UINT * const                     eldSbr,
        UINT * const                     eldSbrRatio
        )
{
    AACENC_ERROR err = AACENC_OK;
    int i, cfgIdx = -1;
    const ULONG channelBitrate = totalBitrate / FDKaacEnc_GetChannelModeConfiguration(channelMode)->nChannelsEff;

    for (i=0; i<(sizeof(eldSbrAutoConfigTab)/sizeof(ELD_SBR_CONFIGURATOR)); i++) {
      if ( (samplingRate <= eldSbrAutoConfigTab[i].samplingRate)
        && (channelBitrate >= eldSbrAutoConfigTab[i].bitrateRange) )
      {
        cfgIdx = i;
      }
    }

    if (cfgIdx != -1) {
      *eldSbr      = (eldSbrAutoConfigTab[cfgIdx].lowDelaySbr==0) ? 0 : 1;
      *eldSbrRatio = (eldSbrAutoConfigTab[cfgIdx].downsampledSbr==0) ? 2 : 1;
    }
    else {
      err = AACENC_INVALID_CONFIG; /* no default configuration for eld-sbr available. */
    }

    return err;
}

static inline INT isSbrActive(const HANDLE_AACENC_CONFIG hAacConfig)
{
    INT sbrUsed = 0;

    if ( (hAacConfig->audioObjectType==AOT_SBR)         || (hAacConfig->audioObjectType==AOT_PS)
      || (hAacConfig->audioObjectType==AOT_MP2_SBR)     || (hAacConfig->audioObjectType==AOT_MP2_PS)
      || (hAacConfig->audioObjectType==AOT_DABPLUS_SBR) || (hAacConfig->audioObjectType==AOT_DABPLUS_PS)
      || (hAacConfig->audioObjectType==AOT_DRM_SBR)     || (hAacConfig->audioObjectType==AOT_DRM_MPEG_PS) )
    {
        sbrUsed = 1;
    }
    if (hAacConfig->audioObjectType == AOT_ER_AAC_ELD && (hAacConfig->syntaxFlags & AC_SBR_PRESENT))
    {
        sbrUsed = 1;
    }

    return ( sbrUsed );
}

static inline INT isPsActive(const AUDIO_OBJECT_TYPE audioObjectType)
{
    INT psUsed = 0;

    if ( (audioObjectType==AOT_PS)
      || (audioObjectType==AOT_MP2_PS)
      || (audioObjectType==AOT_DABPLUS_PS)
      || (audioObjectType==AOT_DRM_MPEG_PS) )
    {
        psUsed = 1;
    }

    return ( psUsed );
}

static SBR_PS_SIGNALING getSbrSignalingMode(
        const AUDIO_OBJECT_TYPE          audioObjectType,
        const TRANSPORT_TYPE             transportType,
        const UCHAR                      transportSignaling,
        const UINT                       sbrRatio
        )

{
  SBR_PS_SIGNALING sbrSignaling;

  if (transportType==TT_UNKNOWN || sbrRatio==0) {
    sbrSignaling = SIG_UNKNOWN; /* Needed parameters have not been set */
    return sbrSignaling;
  } else {
    sbrSignaling = SIG_IMPLICIT; /* default: implicit signaling */
  }

  if ((audioObjectType==AOT_AAC_LC)     || (audioObjectType==AOT_SBR)     || (audioObjectType==AOT_PS)    ||
      (audioObjectType==AOT_MP2_AAC_LC) || (audioObjectType==AOT_MP2_SBR) || (audioObjectType==AOT_MP2_PS) ) {
    switch (transportType) {
      case TT_MP4_ADIF:
      case TT_MP4_ADTS:
        sbrSignaling = SIG_IMPLICIT; /* For MPEG-2 transport types, only implicit signaling is possible */
        break;

      case TT_MP4_RAW:
      case TT_MP4_LATM_MCP1:
      case TT_MP4_LATM_MCP0:
      case TT_MP4_LOAS:
      default:
        if ( transportSignaling==0xFF ) {
          /* Defaults */
          if ( sbrRatio==1 ) {
            sbrSignaling = SIG_EXPLICIT_HIERARCHICAL; /* For downsampled SBR, explicit signaling is mandatory */
          } else {
            sbrSignaling = SIG_IMPLICIT; /* For dual-rate SBR, implicit signaling is default */
          }
        } else {
          /* User set parameters */
          /* Attention: Backward compatible explicit signaling does only work with AMV1 for LATM/LOAS */
          sbrSignaling = (SBR_PS_SIGNALING)transportSignaling;
        }
        break;
    }
  }

  return sbrSignaling;
}

/****************************************************************************
                               Allocate Encoder
****************************************************************************/

H_ALLOC_MEM (_AacEncoder, AACENCODER)
C_ALLOC_MEM (_AacEncoder, AACENCODER, 1)




/*
 * Map Encoder specific config structures to CODER_CONFIG.
 */
static void FDKaacEnc_MapConfig(
        CODER_CONFIG *const              cc,
        const USER_PARAM *const          extCfg,
        const SBR_PS_SIGNALING           sbrSignaling,
        const HANDLE_AACENC_CONFIG       hAacConfig
        )
{
  AUDIO_OBJECT_TYPE transport_AOT = AOT_NULL_OBJECT;
  FDKmemclear(cc, sizeof(CODER_CONFIG));

  cc->flags = 0;

  /* Map virtual aot to transport aot. */
  switch (hAacConfig->audioObjectType) {
    case AOT_MP2_AAC_LC:
      transport_AOT = AOT_AAC_LC;
      break;
    case AOT_MP2_SBR:
      transport_AOT = AOT_SBR;
      cc->flags |= CC_SBR;
     break;
    case AOT_MP2_PS:
      transport_AOT = AOT_PS;
      cc->flags |= CC_SBR;
      break;
    default:
      transport_AOT = hAacConfig->audioObjectType;
  }

  if (hAacConfig->audioObjectType == AOT_ER_AAC_ELD) {
    cc->flags |= (hAacConfig->syntaxFlags & AC_SBR_PRESENT) ? CC_SBR : 0;
  }

  /* transport type is usually AAC-LC. */
  if ( (transport_AOT == AOT_SBR) || (transport_AOT == AOT_PS) ) {
    cc->aot           = AOT_AAC_LC;
  }
  else {
    cc->aot           = transport_AOT;
  }

  /* Configure extension aot. */
  if (sbrSignaling==SIG_IMPLICIT) {
    cc->extAOT = AOT_NULL_OBJECT;  /* implicit */
  }
  else {
    if ( (sbrSignaling==SIG_EXPLICIT_BW_COMPATIBLE) && ( (transport_AOT==AOT_SBR) || (transport_AOT==AOT_PS) ) ) {
      cc->extAOT = AOT_SBR;        /* explicit backward compatible */
    }
    else {
      cc->extAOT = transport_AOT;  /* explicit hierarchical */
    }
  }

  if ( (transport_AOT==AOT_SBR) || (transport_AOT==AOT_PS) ) {
    cc->sbrPresent=1;
    if (transport_AOT==AOT_PS) {
      cc->psPresent=1;
    }
  }
  cc->sbrSignaling    = sbrSignaling;

  cc->extSamplingRate = extCfg->userSamplerate;
  cc->bitRate         = hAacConfig->bitRate;
  cc->noChannels      = hAacConfig->nChannels;
  cc->flags          |= CC_IS_BASELAYER;
  cc->channelMode     = hAacConfig->channelMode;

  cc->nSubFrames = (hAacConfig->nSubFrames > 1 && extCfg->userTpNsubFrames == 1)
                 ? hAacConfig->nSubFrames
                 : extCfg->userTpNsubFrames;

  cc->flags          |= (extCfg->userTpProtection) ? CC_PROTECTION : 0;

  if (extCfg->userTpHeaderPeriod!=0xFF) {
    cc->headerPeriod    = extCfg->userTpHeaderPeriod;
  }
  else { /* auto-mode */
    switch (extCfg->userTpType) {
      case TT_MP4_ADTS:
      case TT_MP4_LOAS:
      case TT_MP4_LATM_MCP1:
        cc->headerPeriod = DEFAULT_HEADER_PERIOD_REPETITION_RATE;
        break;
      default:
        cc->headerPeriod = 0;
    }
  }

  cc->samplesPerFrame = hAacConfig->framelength;
  cc->samplingRate    = hAacConfig->sampleRate;

  /* Mpeg-4 signaling for transport library. */
  switch ( hAacConfig->audioObjectType ) {
    case AOT_MP2_AAC_LC:
    case AOT_MP2_SBR:
    case AOT_MP2_PS:
      cc->flags &= ~CC_MPEG_ID; /* Required for ADTS. */
      cc->extAOT = AOT_NULL_OBJECT;
      break;
    default:
      cc->flags |= CC_MPEG_ID;
  }

  /* ER-tools signaling. */
  cc->flags     |= (hAacConfig->syntaxFlags & AC_ER_VCB11) ? CC_VCB11 : 0;
  cc->flags     |= (hAacConfig->syntaxFlags & AC_ER_HCR)   ? CC_HCR : 0;
  cc->flags     |= (hAacConfig->syntaxFlags & AC_ER_RVLC)  ? CC_RVLC : 0;

  /* Matrix mixdown coefficient configuration. */
  if ( (extCfg->userPceAdditions&0x1) && (hAacConfig->epConfig==-1)
      && ((cc->channelMode==MODE_1_2_2)||(cc->channelMode==MODE_1_2_2_1)) )
  {
    cc->matrixMixdownA       = ((extCfg->userPceAdditions>>1)&0x3)+1;
    cc->flags |= (extCfg->userPceAdditions>>3)&0x1 ? CC_PSEUDO_SURROUND : 0;
  }
  else {
    cc->matrixMixdownA = 0;
  }
}

/*
 * Examine buffer descriptor regarding choosen identifier.
 *
 * \param pBufDesc              Pointer to buffer descriptor
 * \param identifier            Buffer identifier to look for.

 * \return - Buffer descriptor index.
 *         -1, if there is no entry available.
 */
static INT getBufDescIdx(
        const AACENC_BufDesc         *pBufDesc,
        const AACENC_BufferIdentifier identifier
)
{
    INT i, idx = -1;

    for (i=0; i<pBufDesc->numBufs; i++) {
      if ( (AACENC_BufferIdentifier)pBufDesc->bufferIdentifiers[i] == identifier ) {
        idx = i;
        break;
      }
    }
    return idx;
}


/****************************************************************************
                          Function Declarations
****************************************************************************/

AAC_ENCODER_ERROR aacEncDefaultConfig(HANDLE_AACENC_CONFIG hAacConfig,
                                      USER_PARAM *config)
{
    /* make reasonable default settings */
    FDKaacEnc_AacInitDefaultConfig (hAacConfig);

    /* clear configuration structure and copy default settings */
    FDKmemclear(config, sizeof(USER_PARAM));

    /* copy encoder configuration settings */
    config->nChannels       = hAacConfig->nChannels;
    config->userAOT = hAacConfig->audioObjectType = AOT_AAC_LC;
    config->userSamplerate  = hAacConfig->sampleRate;
    config->userChannelMode = hAacConfig->channelMode;
    config->userBitrate     = hAacConfig->bitRate;
    config->userBitrateMode = hAacConfig->bitrateMode;
    config->userBandwidth   = hAacConfig->bandWidth;
    config->userTns         = hAacConfig->useTns;
    config->userPns         = hAacConfig->usePns;
    config->userIntensity   = hAacConfig->useIS;
    config->userAfterburner = hAacConfig->useRequant;
    config->userFramelength = (UINT)-1;

    if (hAacConfig->syntaxFlags & AC_ER_VCB11) {
      config->userErTools  |= 0x01;
    }
    if (hAacConfig->syntaxFlags & AC_ER_HCR) {
      config->userErTools  |= 0x02;
    }

    /* initialize transport parameters */
    config->userTpType         = TT_UNKNOWN;
    config->userTpAmxv         = 0;
    config->userTpSignaling    = 0xFF;    /* choose signaling automatically */
    config->userTpNsubFrames   = 1;
    config->userTpProtection   = 0;    /* not crc protected*/
    config->userTpHeaderPeriod = 0xFF; /* header period in auto mode */
    config->userPceAdditions   = 0;    /* no matrix mixdown coefficient */
    config->userMetaDataMode   = 0;    /* do not embed any meta data info */

    config->userAncDataRate    = 0;

    /* SBR rate is set to 0 here, which means it should be set automatically
       in FDKaacEnc_AdjustEncSettings() if the user did not set a rate
       expilicitely. */
    config->userSbrRatio = 0;

    /* SBR enable set to -1 means to inquire ELD audio configurator for reasonable configuration. */
    config->userSbrEnabled     = -1;

    return AAC_ENC_OK;
}

static
void aacEncDistributeSbrBits(CHANNEL_MAPPING *channelMapping, SBR_ELEMENT_INFO *sbrElInfo, INT bitRate)
{
  INT codebits = bitRate;
  int el;

  /* Copy Element info */
  for (el=0; el<channelMapping->nElements; el++) {
      sbrElInfo[el].ChannelIndex[0] = channelMapping->elInfo[el].ChannelIndex[0];
      sbrElInfo[el].ChannelIndex[1] = channelMapping->elInfo[el].ChannelIndex[1];
      sbrElInfo[el].elType          = channelMapping->elInfo[el].elType;
      sbrElInfo[el].bitRate         = (INT)(fMultNorm(channelMapping->elInfo[el].relativeBits, (FIXP_DBL)bitRate));
      sbrElInfo[el].instanceTag     = channelMapping->elInfo[el].instanceTag;
      sbrElInfo[el].nChannelsInEl   = channelMapping->elInfo[el].nChannelsInEl;

      codebits -= sbrElInfo[el].bitRate;
  }
  sbrElInfo[0].bitRate += codebits;
}


static
INT aacEncoder_LimitBitrate(
        const HANDLE_TRANSPORTENC hTpEnc,
        const INT samplingRate,
        const INT frameLength,
        const INT nChannels,
        const CHANNEL_MODE channelMode,
        INT bitRate,
        const INT nSubFrames,
        const INT sbrActive,
        const INT sbrDownSampleRate,
        const AUDIO_OBJECT_TYPE aot
        )
{
  INT coreSamplingRate;
  CHANNEL_MAPPING cm;

  FDKaacEnc_InitChannelMapping(channelMode, CH_ORDER_MPEG, &cm);

  if (sbrActive) {
    coreSamplingRate = samplingRate >>  (sbrEncoder_IsSingleRatePossible(aot) ? (sbrDownSampleRate-1):1);
  } else {
    coreSamplingRate = samplingRate;
  }

  /* Consider bandwidth channel bit rate limit (see bandwidth.cpp: GetBandwidthEntry()) */
  if (aot == AOT_ER_AAC_LD || aot == AOT_ER_AAC_ELD) {
    bitRate = FDKmin(360000*nChannels, bitRate);
    bitRate = FDKmax(8000*nChannels, bitRate);
  }

  if (aot == AOT_AAC_LC || aot == AOT_SBR || aot == AOT_PS)  {
    bitRate = FDKmin(576000*nChannels, bitRate);
    /*bitRate = FDKmax(0*nChannels, bitRate);*/
  }


  /* Limit bit rate in respect to the core coder */
  bitRate = FDKaacEnc_LimitBitrate(
          hTpEnc,
          coreSamplingRate,
          frameLength,
          nChannels,
          cm.nChannelsEff,
          bitRate,
          -1,
          NULL,
          -1,
          nSubFrames
          );

  /* Limit bit rate in respect to available SBR modes if active */
  if (sbrActive)
  {
    int numIterations = 0;
    INT initialBitrate, adjustedBitrate;
    initialBitrate = adjustedBitrate = bitRate;

    /* Find total bitrate which provides valid configuration for each SBR element. */
    do {
      int e;
      SBR_ELEMENT_INFO sbrElInfo[(8)];
      FDK_ASSERT(cm.nElements <= (8));

      initialBitrate = adjustedBitrate;

      /* Get bit rate for each SBR element */
      aacEncDistributeSbrBits(&cm, sbrElInfo, initialBitrate);

      for (e=0; e<cm.nElements; e++)
      {
        INT sbrElementBitRateIn, sbrBitRateOut;

        if (cm.elInfo[e].elType != ID_SCE && cm.elInfo[e].elType != ID_CPE) {
          continue;
        }
        sbrElementBitRateIn = sbrElInfo[e].bitRate;
        sbrBitRateOut = sbrEncoder_LimitBitRate(sbrElementBitRateIn , cm.elInfo[e].nChannelsInEl, coreSamplingRate, aot);
        if (sbrBitRateOut == 0) {
          return 0;
        }

        /* If bitrates don't match, distribution and limiting needs to be determined again.
           Abort element loop and restart with adapted bitrate. */
        if (sbrElementBitRateIn != sbrBitRateOut) {

          if (sbrElementBitRateIn < sbrBitRateOut) {
            adjustedBitrate = fMax(initialBitrate, (INT)fDivNorm((FIXP_DBL)(sbrBitRateOut+8), cm.elInfo[e].relativeBits));
            break;
          }

          if (sbrElementBitRateIn > sbrBitRateOut) {
            adjustedBitrate = fMin(initialBitrate, (INT)fDivNorm((FIXP_DBL)(sbrBitRateOut-8), cm.elInfo[e].relativeBits));
            break;
          }

        } /* sbrElementBitRateIn != sbrBitRateOut */

      } /* elements */

      numIterations++; /* restrict iteration to worst case of num elements */

    } while ( (initialBitrate!=adjustedBitrate) && (numIterations<=cm.nElements) );

    /* Unequal bitrates mean that no reasonable bitrate configuration found. */
    bitRate = (initialBitrate==adjustedBitrate) ? adjustedBitrate : 0;
  }

  FDK_ASSERT(bitRate > 0);

  return bitRate;
}

/*
 * \brief Consistency check of given USER_PARAM struct and
 *   copy back configuration from public struct into internal
 *   encoder configuration struct.
 *
 * \hAacEncoder Internal encoder config which is to be updated
 * \param config User provided config (public struct)
 * \return ´returns always AAC_ENC_OK
 */
static
AACENC_ERROR FDKaacEnc_AdjustEncSettings(HANDLE_AACENCODER hAacEncoder,
                                         USER_PARAM *config)
{
    AACENC_ERROR err = AACENC_OK;

    /* Get struct pointers. */
    HANDLE_AACENC_CONFIG    hAacConfig = &hAacEncoder->aacConfig;

    hAacConfig->nChannels       = config->nChannels;

    /* Encoder settings update. */
    hAacConfig->sampleRate      = config->userSamplerate;
    hAacConfig->useTns          = config->userTns;
    hAacConfig->usePns          = config->userPns;
    hAacConfig->useIS           = config->userIntensity;
    hAacConfig->bitRate         = config->userBitrate;
    hAacConfig->channelMode     = config->userChannelMode;
    hAacConfig->bitrateMode     = config->userBitrateMode;
    hAacConfig->bandWidth       = config->userBandwidth;
    hAacConfig->useRequant      = config->userAfterburner;

    hAacConfig->audioObjectType = config->userAOT;
    hAacConfig->anc_Rate        = config->userAncDataRate;
    hAacConfig->syntaxFlags     = 0;
    hAacConfig->epConfig        = -1;

    /* Adapt internal AOT when necessary. */
    switch ( hAacConfig->audioObjectType ) {
      case AOT_MP2_AAC_LC:
      case AOT_MP2_SBR:
      case AOT_MP2_PS:
          hAacConfig->usePns = 0;
      case AOT_AAC_LC:
      case AOT_SBR:
      case AOT_PS:
          config->userTpType = (config->userTpType!=TT_UNKNOWN) ? config->userTpType : TT_MP4_ADTS;
          hAacConfig->framelength = (config->userFramelength!=(UINT)-1) ? config->userFramelength : 1024;
          if (hAacConfig->framelength != 1024) {
            return AACENC_INVALID_CONFIG;
          }
          break;
      case AOT_ER_AAC_LD:
          hAacConfig->epConfig = 0;
          hAacConfig->syntaxFlags |= AC_ER|AC_LD;
          hAacConfig->syntaxFlags |= ((config->userErTools & 0x1) ? AC_ER_VCB11 : 0);
          hAacConfig->syntaxFlags |= ((config->userErTools & 0x2) ? AC_ER_HCR : 0);
          hAacConfig->syntaxFlags |= ((config->userErTools & 0x4) ? AC_ER_RVLC : 0);
          config->userTpType = (config->userTpType!=TT_UNKNOWN) ? config->userTpType : TT_MP4_LOAS;
          hAacConfig->framelength = (config->userFramelength!=(UINT)-1) ? config->userFramelength : 512;
          if (hAacConfig->framelength != 512 && hAacConfig->framelength != 480) {
            return AACENC_INVALID_CONFIG;
          }
          break;
      case AOT_ER_AAC_ELD:
          hAacConfig->epConfig = 0;
          hAacConfig->syntaxFlags |= AC_ER|AC_ELD;
          hAacConfig->syntaxFlags |= ((config->userErTools & 0x1) ? AC_ER_VCB11 : 0);
          hAacConfig->syntaxFlags |= ((config->userErTools & 0x2) ? AC_ER_HCR : 0);
          hAacConfig->syntaxFlags |= ((config->userErTools & 0x4) ? AC_ER_RVLC : 0);
          hAacConfig->syntaxFlags |= ((config->userSbrEnabled==1)  ? AC_SBR_PRESENT : 0);
          config->userTpType = (config->userTpType!=TT_UNKNOWN) ? config->userTpType : TT_MP4_LOAS;
          hAacConfig->framelength = (config->userFramelength!=(UINT)-1) ? config->userFramelength : 512;
          if (hAacConfig->framelength != 512 && hAacConfig->framelength != 480) {
            return AACENC_INVALID_CONFIG;
          }
          break;
      default:
          break;
    }

    switch ( hAacConfig->audioObjectType ) {
      case AOT_ER_AAC_LD:
      case AOT_ER_AAC_ELD:
        if (config->userBitrateMode==8) {
          hAacConfig->bitrateMode = 0;
        }
        if (config->userBitrateMode==0) {
          hAacConfig->bitreservoir = 100*config->nChannels; /* default, reduced bitreservoir */
        }
        if (hAacConfig->bitrateMode!=0) {
          return AACENC_INVALID_CONFIG;
        }
        break;
      default:
        break;
    }

    hAacConfig->bitRate = config->userBitrate;

    /* get bitrate in VBR configuration */
    if ( (hAacConfig->bitrateMode>=1) && (hAacConfig->bitrateMode<=5) ) {
        /* In VBR mode; SBR-modul depends on bitrate, core encoder on bitrateMode. */
        hAacConfig->bitRate = FDKaacEnc_GetVBRBitrate(hAacConfig->bitrateMode, hAacConfig->channelMode);
    }



    /* Set default bitrate if no external bitrate declared. */
    if ( (hAacConfig->bitrateMode==0) && (config->userBitrate==(UINT)-1) ) {
        INT bitrate = FDKaacEnc_GetChannelModeConfiguration(hAacConfig->channelMode)->nChannelsEff * hAacConfig->sampleRate;

        if ( isPsActive(hAacConfig->audioObjectType) ) {
          hAacConfig->bitRate = (bitrate>>1);                  /* 0.5 bit per sample */
        }
        else if ( isSbrActive(hAacConfig) )
        {
          if ( (config->userSbrRatio==2) || ((config->userSbrRatio==0)&&(hAacConfig->audioObjectType!=AOT_ER_AAC_ELD)) ) {
            hAacConfig->bitRate = (bitrate + (bitrate>>2))>>1; /* 0.625 bits per sample */
          }
          if ( (config->userSbrRatio==1) || ((config->userSbrRatio==0)&&(hAacConfig->audioObjectType==AOT_ER_AAC_ELD)) ) {
            hAacConfig->bitRate = (bitrate + (bitrate>>3));    /* 1.125 bits per sample */
          }
        } else
        {
                hAacConfig->bitRate = bitrate + (bitrate>>1);        /* 1.5 bits per sample */
        }
    }

    /* Initialize SBR parameters */
    if ( (hAacConfig->audioObjectType==AOT_ER_AAC_ELD)
      && (config->userSbrEnabled == (UCHAR)-1) && (config->userSbrRatio==0) )
    {
      UINT eldSbr = 0;
      UINT eldSbrRatio = 0;

      if ( AACENC_OK!=(err=eldSbrConfigurator(
            hAacConfig->sampleRate,
            hAacConfig->channelMode,
            hAacConfig->bitRate,
           &eldSbr,
           &eldSbrRatio)) )
      {
        return err;
      }

      hAacConfig->syntaxFlags |= ((eldSbr) ? AC_SBR_PRESENT : 0);
      hAacConfig->sbrRatio = eldSbrRatio;
    }
    else
    if ( (config->userSbrRatio==0) && (isSbrActive(hAacConfig)) ) {
      /* Automatic SBR ratio configuration
       * - downsampled SBR for ELD
       * - otherwise always dualrate SBR
       */
        hAacConfig->sbrRatio = (hAacConfig->audioObjectType==AOT_ER_AAC_ELD) ? 1 : 2;
    }
    else {
      /* SBR ratio has been set by the user, so use it. */
      hAacConfig->sbrRatio = config->userSbrRatio;
    }

    {
      UCHAR tpSignaling=getSbrSignalingMode(hAacConfig->audioObjectType, config->userTpType, config->userTpSignaling, hAacConfig->sbrRatio);

      if ( (hAacConfig->audioObjectType==AOT_AAC_LC || hAacConfig->audioObjectType==AOT_SBR || hAacConfig->audioObjectType==AOT_PS) &&
           (config->userTpType==TT_MP4_LATM_MCP1 || config->userTpType==TT_MP4_LATM_MCP0 || config->userTpType==TT_MP4_LOAS) &&
           (tpSignaling==1) && (config->userTpAmxv==0) ) {
             /* For backward compatible explicit signaling, AMV1 has to be active */
             return AACENC_INVALID_CONFIG;
      }

      if ( (hAacConfig->audioObjectType==AOT_AAC_LC || hAacConfig->audioObjectType==AOT_SBR || hAacConfig->audioObjectType==AOT_PS) &&
           (tpSignaling==0) && (hAacConfig->sbrRatio==1)) {
             /* Downsampled SBR has to be signaled explicitely (for transmission of SBR sampling fequency) */
             return AACENC_INVALID_CONFIG;
      }
    }



    /* We need the frame length to call aacEncoder_LimitBitrate() */
    hAacConfig->bitRate = aacEncoder_LimitBitrate(
              NULL,
              hAacConfig->sampleRate,
              hAacConfig->framelength,
              hAacConfig->nChannels,
              hAacConfig->channelMode,
              hAacConfig->bitRate,
              hAacConfig->nSubFrames,
              isSbrActive(hAacConfig),
              hAacConfig->sbrRatio,
              hAacConfig->audioObjectType
              );

    /* Configure PNS */
    if ( ((hAacConfig->bitrateMode>=1) && (hAacConfig->bitrateMode<=5)) /* VBR without PNS. */
        || (hAacConfig->useTns == 0) )                                  /* TNS required. */
    {
        hAacConfig->usePns = 0;
    }

    if (hAacConfig->epConfig >= 0) {
        hAacConfig->syntaxFlags |= AC_ER;
         if (((INT)hAacConfig->channelMode < 1) || ((INT)hAacConfig->channelMode > 7)) {
           return AACENC_INVALID_CONFIG;        /* Cannel config 0 not supported. */
         }
    }

    if ( FDKaacEnc_DetermineEncoderMode(&hAacConfig->channelMode, hAacConfig->nChannels) != AAC_ENC_OK) {
        return AACENC_INVALID_CONFIG;        /* nChannels doesn't match chMode, this is just a check-up */
    }

    if ( (hAacConfig->nChannels > hAacEncoder->nMaxAacChannels)
      || ( (FDKaacEnc_GetChannelModeConfiguration(hAacConfig->channelMode)->nChannelsEff > hAacEncoder->nMaxSbrChannels) &&
            isSbrActive(hAacConfig) )
         )
    {
        return AACENC_INVALID_CONFIG;      /* not enough channels allocated */
    }

    /* Meta data restriction. */
    switch (hAacConfig->audioObjectType)
    {
      /* Allow metadata support */
      case AOT_AAC_LC:
      case AOT_SBR:
      case AOT_PS:
        hAacEncoder->metaDataAllowed = 1;
        if (((INT)hAacConfig->channelMode < 1) || ((INT)hAacConfig->channelMode > 7)) {
          config->userMetaDataMode = 0;
        }
        break;
      /* Prohibit metadata support */
      default:
        hAacEncoder->metaDataAllowed = 0;
    }

    return err;
}

static
INT aacenc_SbrCallback(
        void *                  self,
        HANDLE_FDK_BITSTREAM    hBs,
        const INT sampleRateIn,
        const INT sampleRateOut,
        const INT samplesPerFrame,
        const AUDIO_OBJECT_TYPE coreCodec,
        const MP4_ELEMENT_ID    elementID,
        const INT               elementIndex
        )
{
  HANDLE_AACENCODER hAacEncoder = (HANDLE_AACENCODER)self;

  sbrEncoder_GetHeader(hAacEncoder->hEnvEnc, hBs, elementIndex, 0);

  return 0;
}

static AACENC_ERROR aacEncInit(HANDLE_AACENCODER  hAacEncoder,
                               ULONG              InitFlags,
                               USER_PARAM        *config)
{
    AACENC_ERROR err = AACENC_OK;

    INT aacBufferOffset = 0;
    HANDLE_SBR_ENCODER     *hSbrEncoder = &hAacEncoder->hEnvEnc;
    HANDLE_AACENC_CONFIG    hAacConfig  = &hAacEncoder->aacConfig;

    hAacEncoder->nZerosAppended = 0;          /* count appended zeros */

    INT frameLength = hAacConfig->framelength;

    if ( (InitFlags & AACENC_INIT_CONFIG) )
    {
        CHANNEL_MODE prevChMode = hAacConfig->channelMode;

        /* Verify settings and update: config -> heAacEncoder */
        if ( (err=FDKaacEnc_AdjustEncSettings(hAacEncoder, config)) != AACENC_OK ) {
            return err;
        }
        frameLength = hAacConfig->framelength; /* adapt temporal framelength */

        /* Seamless channel reconfiguration in sbr not fully implemented */
        if ( (prevChMode!=hAacConfig->channelMode) && isSbrActive(hAacConfig) ) {
            InitFlags |= AACENC_INIT_STATES;
        }
    }

    /* Clear input buffer */
    if ( (InitFlags == AACENC_INIT_ALL) ) {
        FDKmemclear(hAacEncoder->inputBuffer, sizeof(INT_PCM)*hAacEncoder->nMaxAacChannels*INPUTBUFFER_SIZE);
    }

    if ( (InitFlags & AACENC_INIT_CONFIG) )
    {
        aacBufferOffset = 0;
        if (hAacConfig->audioObjectType == AOT_ER_AAC_ELD) {
            hAacEncoder->nDelay = DELAY_AACELD(hAacConfig->framelength);
        } else
        {
            hAacEncoder->nDelay = DELAY_AAC(hAacConfig->framelength); /* AAC encoder delay */
        }
        hAacConfig->ancDataBitRate = 0;
    }

    if ( isSbrActive(hAacConfig) &&
        ((InitFlags & AACENC_INIT_CONFIG) || (InitFlags & AACENC_INIT_STATES)) )
    {
        INT sbrError;
        SBR_ELEMENT_INFO sbrElInfo[(8)];
        CHANNEL_MAPPING channelMapping;

        if ( FDKaacEnc_InitChannelMapping(hAacConfig->channelMode,
                                          hAacConfig->channelOrder,
                                         &channelMapping) != AAC_ENC_OK )
        {
            return AACENC_INIT_ERROR;
        }

        /* Check return value and if the SBR encoder can handle enough elements */
        if (channelMapping.nElements > (8)) {
            return AACENC_INIT_ERROR;
        }

        aacEncDistributeSbrBits(&channelMapping, sbrElInfo, hAacConfig->bitRate);

        UINT initFlag = 0;
        initFlag += (InitFlags & AACENC_INIT_STATES) ? 1 : 0;

        /* Let the SBR encoder take a look at the configuration and change if required. */
        sbrError = sbrEncoder_Init(
                                *hSbrEncoder,
                                 sbrElInfo,
                                 channelMapping.nElements,
                                 hAacEncoder->inputBuffer,
                                &hAacConfig->bandWidth,
                                &aacBufferOffset,
                                &hAacConfig->nChannels,
                                &hAacConfig->sampleRate,
                                &hAacConfig->sbrRatio,
                                &frameLength,
                                 hAacConfig->audioObjectType,
                                &hAacEncoder->nDelay,
                                 (hAacConfig->audioObjectType == AOT_ER_AAC_ELD) ? 1 : TRANS_FAC,
                                 (config->userTpHeaderPeriod!=0xFF) ? config->userTpHeaderPeriod : DEFAULT_HEADER_PERIOD_REPETITION_RATE,
                                 initFlag
                                );

        /* Suppress AOT reconfiguration and check error status. */
        if (sbrError) {
            return AACENC_INIT_SBR_ERROR;
        }

        if (hAacConfig->nChannels == 1) {
            hAacConfig->channelMode = MODE_1;
        }

        /* Never use PNS if SBR is active */
        if ( hAacConfig->usePns ) {
           hAacConfig->usePns = 0;
        }

        /* estimated bitrate consumed by SBR or PS */
        hAacConfig->ancDataBitRate = sbrEncoder_GetEstimateBitrate(*hSbrEncoder) ;

    } /* sbr initialization */


    /*
     * Initialize Transport - Module.
     */
    if ( (InitFlags & AACENC_INIT_TRANSPORT) )
    {
        UINT flags = 0;

        FDKaacEnc_MapConfig(
                &hAacEncoder->coderConfig,
                config,
                getSbrSignalingMode(hAacConfig->audioObjectType, config->userTpType, config->userTpSignaling, hAacConfig->sbrRatio),
                hAacConfig);

        /* create flags for transport encoder */
        if (config->userTpAmxv == 1) {
            flags |= TP_FLAG_LATM_AMV;
        }
        /* Clear output buffer */
        FDKmemclear(hAacEncoder->outBuffer, hAacEncoder->outBufferInBytes*sizeof(UCHAR));

        /* Initialize Bitstream encoder */
        if ( transportEnc_Init(hAacEncoder->hTpEnc, hAacEncoder->outBuffer, hAacEncoder->outBufferInBytes, config->userTpType, &hAacEncoder->coderConfig, flags) != 0) {
            return AACENC_INIT_TP_ERROR;
        }

    } /* transport initialization */

    /*
     * Initialize AAC - Core.
     */
    if ( (InitFlags & AACENC_INIT_CONFIG) ||
         (InitFlags & AACENC_INIT_STATES) )
    {
        AAC_ENCODER_ERROR err;
        err = FDKaacEnc_Initialize(hAacEncoder->hAacEnc,
                                   hAacConfig,
                                   hAacEncoder->hTpEnc,
                                   (InitFlags & AACENC_INIT_STATES) ? 1 : 0);

        if (err != AAC_ENC_OK) {
            return AACENC_INIT_AAC_ERROR;
        }

    } /* aac initialization */

    /*
     * Initialize Meta Data - Encoder.
     */
    if ( hAacEncoder->hMetadataEnc && (hAacEncoder->metaDataAllowed!=0) &&
        ((InitFlags & AACENC_INIT_CONFIG) ||(InitFlags & AACENC_INIT_STATES)) )
    {
        INT inputDataDelay = DELAY_AAC(hAacConfig->framelength);

        if ( isSbrActive(hAacConfig) && hSbrEncoder!=NULL) {
          inputDataDelay = hAacConfig->sbrRatio*inputDataDelay + sbrEncoder_GetInputDataDelay(*hSbrEncoder);
        }

        if ( FDK_MetadataEnc_Init(hAacEncoder->hMetadataEnc,
                                 ((InitFlags&AACENC_INIT_STATES) ? 1 : 0),
                                  config->userMetaDataMode,
                                  inputDataDelay,
                                  frameLength,
                                  config->userSamplerate,
                                  config->nChannels,
                                  config->userChannelMode,
                                  hAacConfig->channelOrder) != 0)
        {
            return AACENC_INIT_META_ERROR;
        }

        hAacEncoder->nDelay += FDK_MetadataEnc_GetDelay(hAacEncoder->hMetadataEnc);
    }

    /*
     * Update pointer to working buffer.
     */
    if ( (InitFlags & AACENC_INIT_CONFIG) )
    {
        hAacEncoder->inputBufferOffset = aacBufferOffset;

        hAacEncoder->nSamplesToRead = frameLength * config->nChannels;

        /* Make nDelay comparison compatible with config->nSamplesRead */
        hAacEncoder->nDelay *= config->nChannels;

    } /* parameter changed */

    return AACENC_OK;
}


AACENC_ERROR aacEncOpen(
        HANDLE_AACENCODER        *phAacEncoder,
        const UINT                encModules,
        const UINT                maxChannels
        )
{
    AACENC_ERROR err = AACENC_OK;
    HANDLE_AACENCODER  hAacEncoder = NULL;

    if (phAacEncoder == NULL) {
        err = AACENC_INVALID_HANDLE;
        goto bail;
    }

    /* allocate memory */
    hAacEncoder = Get_AacEncoder();

    if (hAacEncoder == NULL) {
        err = AACENC_MEMORY_ERROR;
        goto bail;
    }

    FDKmemclear(hAacEncoder, sizeof(AACENCODER));

    /* Specify encoder modules to be allocated. */
    if (encModules==0) {
        hAacEncoder->encoder_modis = ENC_MODE_FLAG_AAC;
        hAacEncoder->encoder_modis |= ENC_MODE_FLAG_SBR;
        hAacEncoder->encoder_modis |= ENC_MODE_FLAG_PS;
        hAacEncoder->encoder_modis |= ENC_MODE_FLAG_META;
    }
    else {
       /* consider SAC and PS module */
        hAacEncoder->encoder_modis = encModules;
    }

    /* Determine max channel configuration. */
    if (maxChannels==0) {
        hAacEncoder->nMaxAacChannels = (8);
        hAacEncoder->nMaxSbrChannels = (8);
    }
    else {
        hAacEncoder->nMaxAacChannels = (maxChannels&0x00FF);
        if ( (hAacEncoder->encoder_modis&ENC_MODE_FLAG_SBR) ) {
            hAacEncoder->nMaxSbrChannels = (maxChannels&0xFF00) ? (maxChannels>>8) : hAacEncoder->nMaxAacChannels;
        }

        if ( (hAacEncoder->nMaxAacChannels>(8)) || (hAacEncoder->nMaxSbrChannels>(8)) ) {
            err = AACENC_INVALID_CONFIG;
            goto bail;
        }
    } /* maxChannels==0 */

    /* Max number of elements could be tuned any more. */
    hAacEncoder->nMaxAacElements = fixMin((8), hAacEncoder->nMaxAacChannels);
    hAacEncoder->nMaxSbrElements = fixMin((8), hAacEncoder->nMaxSbrChannels);
    hAacEncoder->nMaxSubFrames = (1);


    /* In case of memory overlay, allocate memory out of libraries */

    hAacEncoder->inputBuffer = (INT_PCM*)FDKcalloc(hAacEncoder->nMaxAacChannels*INPUTBUFFER_SIZE, sizeof(INT_PCM));

    /* Open SBR Encoder */
    if (hAacEncoder->encoder_modis&ENC_MODE_FLAG_SBR) {
        if ( sbrEncoder_Open(&hAacEncoder->hEnvEnc,
                              hAacEncoder->nMaxSbrElements,
                              hAacEncoder->nMaxSbrChannels,
                             (hAacEncoder->encoder_modis&ENC_MODE_FLAG_PS) ? 1 : 0 ) )
        {
          err = AACENC_MEMORY_ERROR;
          goto bail;
        }
    } /* (encoder_modis&ENC_MODE_FLAG_SBR) */


    /* Open Aac Encoder */
    if ( FDKaacEnc_Open(&hAacEncoder->hAacEnc,
                         hAacEncoder->nMaxAacElements,
                         hAacEncoder->nMaxAacChannels,
                         (1)) != AAC_ENC_OK )
    {
        err = AACENC_MEMORY_ERROR;
        goto bail;
    }

    { /* Get bitstream outputbuffer size */
      UINT ld_M;
      for (ld_M=1; (UINT)(1<<ld_M) < (hAacEncoder->nMaxSubFrames*hAacEncoder->nMaxAacChannels*6144)>>3; ld_M++) ;
      hAacEncoder->outBufferInBytes = (1<<ld_M);  /* buffer has to be 2^n */
    }
    hAacEncoder->outBuffer = GetRam_bsOutbuffer();
    if (OUTPUTBUFFER_SIZE < hAacEncoder->outBufferInBytes ) {
      err = AACENC_MEMORY_ERROR;
      goto bail;
    }

    /* Open Meta Data Encoder */
    if (hAacEncoder->encoder_modis&ENC_MODE_FLAG_META) {
      if ( FDK_MetadataEnc_Open(&hAacEncoder->hMetadataEnc) )
      {
        err = AACENC_MEMORY_ERROR;
        goto bail;
      }
    } /* (encoder_modis&ENC_MODE_FLAG_META) */

    /* Open Transport Encoder */
    if ( transportEnc_Open(&hAacEncoder->hTpEnc) != 0 )
    {
        err = AACENC_MEMORY_ERROR;
        goto bail;
    }
    else {
        C_ALLOC_SCRATCH_START(pLibInfo, LIB_INFO, FDK_MODULE_LAST);

        FDKinitLibInfo( pLibInfo);
        transportEnc_GetLibInfo( pLibInfo );

        /* Get capabilty flag for transport encoder. */
        hAacEncoder->CAPF_tpEnc = FDKlibInfo_getCapabilities( pLibInfo, FDK_TPENC);

        C_ALLOC_SCRATCH_END(pLibInfo, LIB_INFO, FDK_MODULE_LAST);
    }
    if ( transportEnc_RegisterSbrCallback(hAacEncoder->hTpEnc, aacenc_SbrCallback, hAacEncoder) != 0 ) {
      err = AACENC_INIT_TP_ERROR;
      goto bail;
    }

    /* Initialize encoder instance with default parameters. */
    aacEncDefaultConfig(&hAacEncoder->aacConfig, &hAacEncoder->extParam);

    /* Initialize headerPeriod in coderConfig for aacEncoder_GetParam(). */
    hAacEncoder->coderConfig.headerPeriod = hAacEncoder->extParam.userTpHeaderPeriod;

    /* All encoder modules have to be initialized */
    hAacEncoder->InitFlags = AACENC_INIT_ALL;

    /* Return encoder instance */
    *phAacEncoder = hAacEncoder;

    return err;

bail:
    aacEncClose(&hAacEncoder);

    return err;
}



AACENC_ERROR aacEncClose(HANDLE_AACENCODER *phAacEncoder)
{
    AACENC_ERROR err = AACENC_OK;

    if (phAacEncoder == NULL) {
        err = AACENC_INVALID_HANDLE;
        goto bail;
    }

    if (*phAacEncoder != NULL) {
        HANDLE_AACENCODER hAacEncoder = *phAacEncoder;


       if (hAacEncoder->inputBuffer!=NULL) {
           FDKfree(hAacEncoder->inputBuffer);
           hAacEncoder->inputBuffer = NULL;
       }

       if (hAacEncoder->outBuffer) {
         FreeRam_bsOutbuffer(&hAacEncoder->outBuffer);
       }

        if (hAacEncoder->hEnvEnc) {
            sbrEncoder_Close (&hAacEncoder->hEnvEnc);
        }
        if (hAacEncoder->hAacEnc) {
            FDKaacEnc_Close (&hAacEncoder->hAacEnc);
        }

        transportEnc_Close(&hAacEncoder->hTpEnc);

        if (hAacEncoder->hMetadataEnc) {
            FDK_MetadataEnc_Close (&hAacEncoder->hMetadataEnc);
        }

        Free_AacEncoder(phAacEncoder);
    }

bail:
    return err;
}

AACENC_ERROR aacEncEncode(
        const HANDLE_AACENCODER   hAacEncoder,
        const AACENC_BufDesc     *inBufDesc,
        const AACENC_BufDesc     *outBufDesc,
        const AACENC_InArgs      *inargs,
        AACENC_OutArgs           *outargs
        )
{
    AACENC_ERROR err = AACENC_OK;
    INT i, nBsBytes = 0;
    INT  outBytes[(1)];
    int  nExtensions = 0;
    int  ancDataExtIdx = -1;

    /* deal with valid encoder handle */
    if (hAacEncoder==NULL) {
        err = AACENC_INVALID_HANDLE;
        goto bail;
    }


    /*
     * Adjust user settings and trigger reinitialization.
     */
    if (hAacEncoder->InitFlags!=0) {

        err = aacEncInit(hAacEncoder,
                         hAacEncoder->InitFlags,
                        &hAacEncoder->extParam);

        if (err!=AACENC_OK) {
            /* keep init flags alive! */
            goto bail;
        }
        hAacEncoder->InitFlags = AACENC_INIT_NONE;
    }

    if (outargs!=NULL) {
        FDKmemclear(outargs, sizeof(AACENC_OutArgs));
    }

    if (outBufDesc!=NULL) {
      for (i=0; i<outBufDesc->numBufs; i++) {
        if (outBufDesc->bufs[i]!=NULL) {
          FDKmemclear(outBufDesc->bufs[i], outBufDesc->bufSizes[i]);
        }
      }
    }

    /*
     * If only encoder handle given, independent (re)initialization can be triggered.
     */
    if ( (hAacEncoder!=NULL) & (inBufDesc==NULL) && (outBufDesc==NULL) && (inargs==NULL) && (outargs==NULL) ) {
        goto bail;
    }

    /* reset buffer wich signals number of valid bytes in output bitstream buffer */
    FDKmemclear(outBytes, hAacEncoder->aacConfig.nSubFrames*sizeof(INT));

    /*
     * Manage incoming audio samples.
     */
    if ( (inargs->numInSamples > 0) && (getBufDescIdx(inBufDesc,IN_AUDIO_DATA) != -1) )
    {
        /* Fetch data until nSamplesToRead reached */
        INT idx = getBufDescIdx(inBufDesc,IN_AUDIO_DATA);
        INT newSamples = fixMax(0,fixMin(inargs->numInSamples, hAacEncoder->nSamplesToRead-hAacEncoder->nSamplesRead));
        INT_PCM *pIn = hAacEncoder->inputBuffer+hAacEncoder->inputBufferOffset+hAacEncoder->nSamplesRead;

        /* Copy new input samples to internal buffer */
        if (inBufDesc->bufElSizes[idx]==(INT)sizeof(INT_PCM)) {
            FDKmemcpy(pIn, (INT_PCM*)inBufDesc->bufs[idx], newSamples*sizeof(INT_PCM));  /* Fast copy. */
        }
        else if (inBufDesc->bufElSizes[idx]>(INT)sizeof(INT_PCM)) {
            for (i=0; i<newSamples; i++) {
                pIn[i] = (INT_PCM)(((LONG*)inBufDesc->bufs[idx])[i]>>16);                /* Convert 32 to 16 bit. */
            }
        }
        else {
            for (i=0; i<newSamples; i++) {
                pIn[i] = ((INT_PCM)(((SHORT*)inBufDesc->bufs[idx])[i]))<<16;             /* Convert 16 to 32 bit. */
            }
        }
        hAacEncoder->nSamplesRead += newSamples;

        /* Number of fetched input buffer samples. */
        outargs->numInSamples = newSamples;
    }

    /* input buffer completely filled ? */
    if (hAacEncoder->nSamplesRead < hAacEncoder->nSamplesToRead)
    {
        /* - eof reached and flushing enabled, or
           - return to main and wait for further incoming audio samples */
        if (inargs->numInSamples==-1)
        {
            if ( (hAacEncoder->nZerosAppended < hAacEncoder->nDelay)
                )
            {
              int nZeros = hAacEncoder->nSamplesToRead - hAacEncoder->nSamplesRead;

              FDK_ASSERT(nZeros >= 0);

              /* clear out until end-of-buffer */
              if (nZeros) {
                FDKmemclear(hAacEncoder->inputBuffer+hAacEncoder->inputBufferOffset+hAacEncoder->nSamplesRead, sizeof(INT_PCM)*nZeros );
                hAacEncoder->nZerosAppended += nZeros;
                hAacEncoder->nSamplesRead = hAacEncoder->nSamplesToRead;
              }
            }
            else { /* flushing completed */
              err = AACENC_ENCODE_EOF; /* eof reached */
              goto bail;
            }
        }
        else { /* inargs->numInSamples!= -1 */
            goto bail; /* not enough samples in input buffer and no flushing enabled */
        }
    }

    /* init payload */
    FDKmemclear(hAacEncoder->extPayload, sizeof(AACENC_EXT_PAYLOAD) * MAX_TOTAL_EXT_PAYLOADS);
    for (i = 0; i < MAX_TOTAL_EXT_PAYLOADS; i++) {
      hAacEncoder->extPayload[i].associatedChElement = -1;
    }
    FDKmemclear(hAacEncoder->extPayloadData, sizeof(hAacEncoder->extPayloadData));
    FDKmemclear(hAacEncoder->extPayloadSize, sizeof(hAacEncoder->extPayloadSize));


    /*
     * Calculate Meta Data info.
     */
    if ( (hAacEncoder->hMetadataEnc!=NULL) && (hAacEncoder->metaDataAllowed!=0) ) {

        const AACENC_MetaData *pMetaData = NULL;
        AACENC_EXT_PAYLOAD *pMetaDataExtPayload = NULL;
        UINT nMetaDataExtensions = 0;
        INT  matrix_mixdown_idx = 0;

        /* New meta data info available ? */
        if ( getBufDescIdx(inBufDesc,IN_METADATA_SETUP) != -1 ) {
          pMetaData = (AACENC_MetaData*)inBufDesc->bufs[getBufDescIdx(inBufDesc,IN_METADATA_SETUP)];
        }

        FDK_MetadataEnc_Process(hAacEncoder->hMetadataEnc,
                                hAacEncoder->inputBuffer+hAacEncoder->inputBufferOffset,
                                hAacEncoder->nSamplesRead,
                                pMetaData,
                               &pMetaDataExtPayload,
                               &nMetaDataExtensions,
                               &matrix_mixdown_idx
                                );

        for (i=0; i<(INT)nMetaDataExtensions; i++) {  /* Get meta data extension payload. */
            hAacEncoder->extPayload[nExtensions++] = pMetaDataExtPayload[i];
        }

        if ( (matrix_mixdown_idx!=-1)
          && ((hAacEncoder->extParam.userChannelMode==MODE_1_2_2)||(hAacEncoder->extParam.userChannelMode==MODE_1_2_2_1)) )
        {
          /* Set matrix mixdown coefficient. */
          UINT pceValue = (UINT)( (1<<3) | ((matrix_mixdown_idx&0x3)<<1) | 1 );
          if (hAacEncoder->extParam.userPceAdditions != pceValue) {
            hAacEncoder->extParam.userPceAdditions = pceValue;
            hAacEncoder->InitFlags |= AACENC_INIT_TRANSPORT;
          }
        }
    }


    if ( isSbrActive(&hAacEncoder->aacConfig) ) {

        INT nPayload = 0;

        /*
         * Encode SBR data.
         */
        if (sbrEncoder_EncodeFrame(hAacEncoder->hEnvEnc,
                                   hAacEncoder->inputBuffer,
                                   hAacEncoder->extParam.nChannels,
                                   hAacEncoder->extPayloadSize[nPayload],
                                   hAacEncoder->extPayloadData[nPayload]
#if defined(EVAL_PACKAGE_SILENCE) || defined(EVAL_PACKAGE_SBR_SILENCE)
                                  ,hAacEncoder->hAacEnc->clearOutput
#endif
                                  ))
        {
            err = AACENC_ENCODE_ERROR;
            goto bail;
        }
        else {
            /* Add SBR extension payload */
            for (i = 0; i < (8); i++) {
                if (hAacEncoder->extPayloadSize[nPayload][i] > 0) {
                    hAacEncoder->extPayload[nExtensions].pData    = hAacEncoder->extPayloadData[nPayload][i];
                    {
                      hAacEncoder->extPayload[nExtensions].dataSize = hAacEncoder->extPayloadSize[nPayload][i];
                      hAacEncoder->extPayload[nExtensions].associatedChElement = i;
                    }
                    hAacEncoder->extPayload[nExtensions].dataType = EXT_SBR_DATA;  /* Once SBR Encoder supports SBR CRC set EXT_SBR_DATA_CRC */
                    nExtensions++;                                                 /* or EXT_SBR_DATA according to configuration. */
                    FDK_ASSERT(nExtensions<=MAX_TOTAL_EXT_PAYLOADS);
                }
            }
            nPayload++;
        }
    } /* sbrEnabled */

    if ( (inargs->numAncBytes > 0) && ( getBufDescIdx(inBufDesc,IN_ANCILLRY_DATA)!=-1 ) ) {
        INT idx = getBufDescIdx(inBufDesc,IN_ANCILLRY_DATA);
        hAacEncoder->extPayload[nExtensions].dataSize = inargs->numAncBytes * 8;
        hAacEncoder->extPayload[nExtensions].pData    = (UCHAR*)inBufDesc->bufs[idx];
        hAacEncoder->extPayload[nExtensions].dataType = EXT_DATA_ELEMENT;
        hAacEncoder->extPayload[nExtensions].associatedChElement = -1;
        ancDataExtIdx = nExtensions; /* store index */
        nExtensions++;
    }

    /*
     * Encode AAC - Core.
     */
    if ( FDKaacEnc_EncodeFrame( hAacEncoder->hAacEnc,
                                hAacEncoder->hTpEnc,
                                hAacEncoder->inputBuffer,
                                outBytes,
                                hAacEncoder->extPayload
                                ) != AAC_ENC_OK )
    {
        err = AACENC_ENCODE_ERROR;
        goto bail;
    }

    if (ancDataExtIdx >= 0) {
      outargs->numAncBytes = inargs->numAncBytes - (hAacEncoder->extPayload[ancDataExtIdx].dataSize>>3);
    }

    /* samples exhausted */
    hAacEncoder->nSamplesRead -= hAacEncoder->nSamplesToRead;

    /*
     * Delay balancing buffer handling
     */
    if (isSbrActive(&hAacEncoder->aacConfig)) {
        sbrEncoder_UpdateBuffers(hAacEncoder->hEnvEnc, hAacEncoder->inputBuffer);
    }

    /*
     * Make bitstream public
     */
    if (outBufDesc->numBufs>=1) {

        INT bsIdx = getBufDescIdx(outBufDesc,OUT_BITSTREAM_DATA);
        INT auIdx = getBufDescIdx(outBufDesc,OUT_AU_SIZES);

        for (i=0,nBsBytes=0; i<hAacEncoder->aacConfig.nSubFrames; i++) {
          nBsBytes += outBytes[i];

          if (auIdx!=-1) {
           ((INT*)outBufDesc->bufs[auIdx])[i] = outBytes[i];
          }
        }

        if ( (bsIdx!=-1) && (outBufDesc->bufSizes[bsIdx]>=nBsBytes) ) {
          FDKmemcpy(outBufDesc->bufs[bsIdx], hAacEncoder->outBuffer, sizeof(UCHAR)*nBsBytes);
          outargs->numOutBytes = nBsBytes;
        }
        else {
          /* output buffer too small, can't write valid bitstream */
          err = AACENC_ENCODE_ERROR;
          goto bail;
        }
    }

bail:
    if (err == AACENC_ENCODE_ERROR) {
        /* All encoder modules have to be initialized */
        hAacEncoder->InitFlags = AACENC_INIT_ALL;
    }

    return err;
}

static
AAC_ENCODER_ERROR aacEncGetConf(HANDLE_AACENCODER  hAacEncoder,
                                UINT              *size,
                                UCHAR             *confBuffer)
{
    FDK_BITSTREAM tmpConf;
    UINT confType;
    UCHAR buf[64];
    int err;

    /* Init bit buffer */
    FDKinitBitStream(&tmpConf, buf, 64, 0, BS_WRITER);

    /* write conf in tmp buffer */
    err = transportEnc_GetConf(hAacEncoder->hTpEnc, &hAacEncoder->coderConfig, &tmpConf, &confType);

    /* copy data to outbuffer: length in bytes */
    FDKbyteAlign(&tmpConf, 0);

    /* Check buffer size */
    if (FDKgetValidBits(&tmpConf) > ((*size)<<3))
      return AAC_ENC_UNKNOWN;

    FDKfetchBuffer(&tmpConf, confBuffer, size);

    if (err != 0)
      return AAC_ENC_UNKNOWN;
    else
      return AAC_ENC_OK;
}


AACENC_ERROR aacEncGetLibInfo(LIB_INFO *info)
{
  int i = 0;

  if (info == NULL) {
    return AACENC_INVALID_HANDLE;
  }

  FDK_toolsGetLibInfo( info );
  transportEnc_GetLibInfo( info );

  sbrEncoder_GetLibInfo( info );

  /* search for next free tab */
  for (i = 0; i < FDK_MODULE_LAST; i++) {
    if (info[i].module_id == FDK_NONE) break;
  }
  if (i == FDK_MODULE_LAST) {
    return AACENC_INIT_ERROR;
  }

  info[i].module_id = FDK_AACENC;
  info[i].build_date = (char*)AACENCODER_LIB_BUILD_DATE;
  info[i].build_time = (char*)AACENCODER_LIB_BUILD_TIME;
  info[i].title = (char*)AACENCODER_LIB_TITLE;
  info[i].version = LIB_VERSION(AACENCODER_LIB_VL0, AACENCODER_LIB_VL1, AACENCODER_LIB_VL2);;
  LIB_VERSION_STRING(&info[i]);

  /* Capability flags */
  info[i].flags = 0
    | CAPF_AAC_1024 | CAPF_AAC_LC
    | CAPF_AAC_512
    | CAPF_AAC_480
    | CAPF_AAC_DRC
      ;
  /* End of flags */

  return AACENC_OK;
}

AACENC_ERROR aacEncoder_SetParam(
        const HANDLE_AACENCODER   hAacEncoder,
        const AACENC_PARAM        param,
        const UINT                value
        )
{
    AACENC_ERROR err = AACENC_OK;
    USER_PARAM *settings = &hAacEncoder->extParam;

    /* check encoder handle */
    if (hAacEncoder == NULL) {
        err = AACENC_INVALID_HANDLE;
        goto bail;
    }

    /* apply param value */
    switch (param)
    {
    case AACENC_AOT:
        if (settings->userAOT != (AUDIO_OBJECT_TYPE)value) {
            /* check if AOT matches the allocated modules */
            switch ( value ) {
              case AOT_PS:
              case AOT_MP2_PS:
                if (!(hAacEncoder->encoder_modis & (ENC_MODE_FLAG_PS))) {
                  err = AACENC_INVALID_CONFIG;
                  goto bail;
                }
              case AOT_SBR:
              case AOT_MP2_SBR:
                if (!(hAacEncoder->encoder_modis & (ENC_MODE_FLAG_SBR))) {
                  err = AACENC_INVALID_CONFIG;
                  goto bail;
                }
              case AOT_AAC_LC:
              case AOT_MP2_AAC_LC:
              case AOT_ER_AAC_LD:
              case AOT_ER_AAC_ELD:
                if (!(hAacEncoder->encoder_modis & (ENC_MODE_FLAG_AAC))) {
                  err = AACENC_INVALID_CONFIG;
                  goto bail;
                }
                break;
              default:
                err = AACENC_INVALID_CONFIG;
                goto bail;
            }/* switch value */
            settings->userAOT = (AUDIO_OBJECT_TYPE)value;
            hAacEncoder->InitFlags |= AACENC_INIT_CONFIG | AACENC_INIT_STATES | AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_BITRATE:
        if (settings->userBitrate != value) {
            settings->userBitrate = value;
            hAacEncoder->InitFlags |= AACENC_INIT_CONFIG | AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_BITRATEMODE:
        if (settings->userBitrateMode != value) {
            switch ( value ) {
              case 0:
              case 8:
                settings->userBitrateMode = value;
                hAacEncoder->InitFlags |= AACENC_INIT_CONFIG | AACENC_INIT_TRANSPORT;
                break;
              default:
                err = AACENC_INVALID_CONFIG;
                break;
            } /* switch value */
        }
        break;
    case AACENC_SAMPLERATE:
        if (settings->userSamplerate != value) {
            if ( !( (value==8000) || (value==11025) || (value==12000) || (value==16000) || (value==22050) || (value==24000) ||
                   (value==32000) || (value==44100) || (value==48000) || (value==64000) || (value==88200) || (value==96000) ) )
            {
                err = AACENC_INVALID_CONFIG;
                break;
            }
            settings->userSamplerate = value;
            hAacEncoder->nSamplesRead = 0; /* reset internal inputbuffer */
            hAacEncoder->InitFlags |= AACENC_INIT_CONFIG | AACENC_INIT_STATES | AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_CHANNELMODE:
        if (settings->userChannelMode != (CHANNEL_MODE)value) {
            const CHANNEL_MODE_CONFIG_TAB* pConfig = FDKaacEnc_GetChannelModeConfiguration((CHANNEL_MODE)value);
            if (pConfig==NULL) {
                err = AACENC_INVALID_CONFIG;
                break;
            }
            if ( (pConfig->nElements > hAacEncoder->nMaxAacElements)
              || (pConfig->nChannelsEff > hAacEncoder->nMaxAacChannels)
              || !(((value>=1) && (value<=7))||((value>=33) && (value<=34)))
                )
            {
                err = AACENC_INVALID_CONFIG;
                break;
            }

            settings->userChannelMode = (CHANNEL_MODE)value;
            settings->nChannels = pConfig->nChannels;
            hAacEncoder->nSamplesRead = 0; /* reset internal inputbuffer */
            hAacEncoder->InitFlags |= AACENC_INIT_CONFIG | AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_BANDWIDTH:
        if (settings->userBandwidth != value) {
          settings->userBandwidth = value;
          hAacEncoder->InitFlags |= AACENC_INIT_CONFIG;
        }
        break;
    case AACENC_CHANNELORDER:
        if (hAacEncoder->aacConfig.channelOrder != (CHANNEL_ORDER)value) {
            if (! ((value==0) || (value==1)) ) {
                err = AACENC_INVALID_CONFIG;
                break;
            }
            hAacEncoder->aacConfig.channelOrder = (CHANNEL_ORDER)value;
            hAacEncoder->nSamplesRead = 0; /* reset internal inputbuffer */
            hAacEncoder->InitFlags |= AACENC_INIT_CONFIG | AACENC_INIT_STATES | AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_AFTERBURNER:
        if (settings->userAfterburner != value) {
            if (! ((value==0) || (value==1)) ) {
                err = AACENC_INVALID_CONFIG;
                break;
            }
            settings->userAfterburner = value;
            hAacEncoder->InitFlags |= AACENC_INIT_CONFIG;
        }
        break;
    case AACENC_GRANULE_LENGTH:
        if (settings->userFramelength != value) {
          switch (value) {
            case 1024:
            case 512:
            case 480:
              settings->userFramelength = value;
              hAacEncoder->InitFlags |= AACENC_INIT_CONFIG | AACENC_INIT_TRANSPORT;
              break;
            default:
              err = AACENC_INVALID_CONFIG;
              break;
          }
        }
        break;
    case AACENC_SBR_RATIO:
        if (settings->userSbrRatio != value) {
            if (! ((value==0) || (value==1) || (value==2)) ) {
              err = AACENC_INVALID_CONFIG;
              break;
            }
            settings->userSbrRatio = value;
            hAacEncoder->InitFlags |= AACENC_INIT_CONFIG | AACENC_INIT_STATES | AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_SBR_MODE:
        if (settings->userSbrEnabled != value) {
            settings->userSbrEnabled = value;
            hAacEncoder->InitFlags |= AACENC_INIT_CONFIG | AACENC_INIT_STATES | AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_TRANSMUX:
        if (settings->userTpType != (TRANSPORT_TYPE)value) {

            TRANSPORT_TYPE  type  = (TRANSPORT_TYPE)value;
            UINT            flags = hAacEncoder->CAPF_tpEnc;

            if ( !( ((type==TT_MP4_ADIF)      &&  (flags&CAPF_ADIF))
                 || ((type==TT_MP4_ADTS)      &&  (flags&CAPF_ADTS))
                 || ((type==TT_MP4_LATM_MCP0) && ((flags&CAPF_LATM) && (flags&CAPF_RAWPACKETS)))
                 || ((type==TT_MP4_LATM_MCP1) && ((flags&CAPF_LATM) && (flags&CAPF_RAWPACKETS)))
                 || ((type==TT_MP4_LOAS)      &&  (flags&CAPF_LOAS))
                 || ((type==TT_MP4_RAW)       &&  (flags&CAPF_RAWPACKETS))
                ) )
            {
                err = AACENC_INVALID_CONFIG;
                break;
            }
            settings->userTpType = (TRANSPORT_TYPE)value;
            hAacEncoder->InitFlags |= AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_SIGNALING_MODE:
        if (settings->userTpSignaling != value) {
            if ( !((value==0) || (value==1) || (value==2)) ) {
                err = AACENC_INVALID_CONFIG;
                break;
            }
            settings->userTpSignaling = value;
            hAacEncoder->InitFlags |= AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_PROTECTION:
        if (settings->userTpProtection != value) {
            if ( !((value==0) || (value==1)) ) {
                err = AACENC_INVALID_CONFIG;
                break;
            }
            settings->userTpProtection = value;
            hAacEncoder->InitFlags |= AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_HEADER_PERIOD:
        if (settings->userTpHeaderPeriod != value) {
            settings->userTpHeaderPeriod = value;
            hAacEncoder->InitFlags |= AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_TPSUBFRAMES:
        if (settings->userTpNsubFrames != value) {
            if (! ( (value>=1) && (value<=4) ) ) {
                err = AACENC_INVALID_CONFIG;
                break;
            }
            settings->userTpNsubFrames = value;
            hAacEncoder->InitFlags |= AACENC_INIT_TRANSPORT;
        }
        break;
    case AACENC_ANCILLARY_BITRATE:
        if (settings->userAncDataRate != value) {
            settings->userAncDataRate = value;
        }
        break;
    case AACENC_CONTROL_STATE:
        if (hAacEncoder->InitFlags != value) {
            if (value&AACENC_RESET_INBUFFER) {
                hAacEncoder->nSamplesRead = 0;
            }
            hAacEncoder->InitFlags = value;
        }
        break;
    case AACENC_METADATA_MODE:
        if ((UINT)settings->userMetaDataMode != value) {
            if ( !((value>=0) && (value<=2)) ) {
                err = AACENC_INVALID_CONFIG;
                break;
            }
            settings->userMetaDataMode = value;
            hAacEncoder->InitFlags |= AACENC_INIT_CONFIG;
        }
        break;
    default:
      err = AACENC_UNSUPPORTED_PARAMETER;
      break;
    }  /* switch(param) */

bail:
    return err;
}

UINT aacEncoder_GetParam(
        const HANDLE_AACENCODER   hAacEncoder,
        const AACENC_PARAM        param
        )
{
    UINT value = 0;
    USER_PARAM *settings = &hAacEncoder->extParam;

    /* check encoder handle */
    if (hAacEncoder == NULL) {
        goto bail;
    }

    /* apply param value */
    switch (param)
    {
    case AACENC_AOT:
        value = (UINT)hAacEncoder->aacConfig.audioObjectType;
        break;
    case AACENC_BITRATE:
        value = (UINT)((hAacEncoder->aacConfig.bitrateMode==AACENC_BR_MODE_CBR) ? hAacEncoder->aacConfig.bitRate : -1);
        break;
    case AACENC_BITRATEMODE:
        value = (UINT)hAacEncoder->aacConfig.bitrateMode;
        break;
    case AACENC_SAMPLERATE:
        value = (UINT)hAacEncoder->coderConfig.extSamplingRate;
        break;
    case AACENC_CHANNELMODE:
        value = (UINT)hAacEncoder->aacConfig.channelMode;
        break;
    case AACENC_BANDWIDTH:
        value = (UINT)hAacEncoder->aacConfig.bandWidth;
        break;
    case AACENC_CHANNELORDER:
        value = (UINT)hAacEncoder->aacConfig.channelOrder;
        break;
    case AACENC_AFTERBURNER:
        value = (UINT)hAacEncoder->aacConfig.useRequant;
        break;
    case AACENC_GRANULE_LENGTH:
        value = (UINT)hAacEncoder->aacConfig.framelength;
       break;
    case AACENC_SBR_RATIO:
        value = isSbrActive(&hAacEncoder->aacConfig) ? hAacEncoder->aacConfig.sbrRatio : 0;
        break;
    case AACENC_SBR_MODE:
        value = (UINT) (hAacEncoder->aacConfig.syntaxFlags & AC_SBR_PRESENT) ? 1 : 0;
        break;
    case AACENC_TRANSMUX:
        value = (UINT)settings->userTpType;
        break;
    case AACENC_SIGNALING_MODE:
        value = (UINT)getSbrSignalingMode(hAacEncoder->aacConfig.audioObjectType, settings->userTpType, settings->userTpSignaling, hAacEncoder->aacConfig.sbrRatio);
        break;
    case AACENC_PROTECTION:
        value = (UINT)settings->userTpProtection;
        break;
    case AACENC_HEADER_PERIOD:
        value = (UINT)hAacEncoder->coderConfig.headerPeriod;
        break;
    case AACENC_TPSUBFRAMES:
        value = (UINT)settings->userTpNsubFrames;
        break;
    case AACENC_ANCILLARY_BITRATE:
        value = (UINT)hAacEncoder->aacConfig.anc_Rate;
        break;
    case AACENC_CONTROL_STATE:
        value = (UINT)hAacEncoder->InitFlags;
        break;
    case AACENC_METADATA_MODE:
        value = (hAacEncoder->metaDataAllowed==0) ? 0 : (UINT)settings->userMetaDataMode;
        break;
    default:
      //err = MPS_INVALID_PARAMETER;
      break;
    }  /* switch(param) */

bail:
    return value;
}

AACENC_ERROR aacEncInfo(
        const HANDLE_AACENCODER   hAacEncoder,
        AACENC_InfoStruct        *pInfo
        )
{
    AACENC_ERROR err = AACENC_OK;

    FDKmemclear(pInfo, sizeof(AACENC_InfoStruct));
    pInfo->confSize = 64; /* pre-initialize */

    pInfo->maxOutBufBytes    = ((hAacEncoder->nMaxAacChannels*6144)+7)>>3;
    pInfo->maxAncBytes       = hAacEncoder->aacConfig.maxAncBytesPerAU;
    pInfo->inBufFillLevel    = hAacEncoder->nSamplesRead/hAacEncoder->extParam.nChannels;
    pInfo->inputChannels     = hAacEncoder->extParam.nChannels;
    pInfo->frameLength       = hAacEncoder->nSamplesToRead/hAacEncoder->extParam.nChannels;
    pInfo->encoderDelay      = hAacEncoder->nDelay/hAacEncoder->extParam.nChannels;

    /* Get encoder configuration */
    if ( aacEncGetConf(hAacEncoder, &pInfo->confSize, &pInfo->confBuf[0]) != AAC_ENC_OK) {
        err = AACENC_INIT_ERROR;
        goto bail;
    }
bail:
    return err;
}

