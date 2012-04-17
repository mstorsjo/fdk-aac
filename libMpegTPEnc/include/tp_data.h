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
   Author(s): Manuel Jander
   Description: MPEG Transport data tables

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
#ifndef __TP_DATA_H__
#define __TP_DATA_H__

#include "machine_type.h"
#include "FDK_audio.h"
#include "FDK_bitstream.h"

/*
 * Configuration
 */
#define TP_GA_ENABLE
/* #define TP_CELP_ENABLE */
/* #define TP_HVXC_ENABLE */
/* #define TP_SLS_ENABLE */
#define TP_ELD_ENABLE
/* #define TP_USAC_ENABLE */
/* #define TP_RSVD50_ENABLE */

#if defined(TP_GA_ENABLE) || defined(TP_SLS_ENABLE)
#define TP_PCE_ENABLE       /**< Enable full PCE support */
#endif

/**
 * ProgramConfig struct.
 */
/* ISO/IEC 14496-3 4.4.1.1 Table 4.2 Program config element */
#define PC_FSB_CHANNELS_MAX 16 /* Front/Side/Back channels */
#define PC_LFE_CHANNELS_MAX 4
#define PC_ASSOCDATA_MAX    8
#define PC_CCEL_MAX         16 /* CC elements */
#define PC_COMMENTLENGTH    256

typedef struct
{
#ifdef TP_PCE_ENABLE
  /* PCE bitstream elements: */
  UCHAR ElementInstanceTag;
  UCHAR Profile;
  UCHAR SamplingFrequencyIndex;
  UCHAR NumFrontChannelElements;
  UCHAR NumSideChannelElements;
  UCHAR NumBackChannelElements;
  UCHAR NumLfeChannelElements;
  UCHAR NumAssocDataElements;
  UCHAR NumValidCcElements;

  UCHAR MonoMixdownPresent;
  UCHAR MonoMixdownElementNumber;

  UCHAR StereoMixdownPresent;
  UCHAR StereoMixdownElementNumber;

  UCHAR MatrixMixdownIndexPresent;
  UCHAR MatrixMixdownIndex;
  UCHAR PseudoSurroundEnable;

  UCHAR FrontElementIsCpe[PC_FSB_CHANNELS_MAX];
  UCHAR FrontElementTagSelect[PC_FSB_CHANNELS_MAX];

  UCHAR SideElementIsCpe[PC_FSB_CHANNELS_MAX];
  UCHAR SideElementTagSelect[PC_FSB_CHANNELS_MAX];

  UCHAR BackElementIsCpe[PC_FSB_CHANNELS_MAX];
  UCHAR BackElementTagSelect[PC_FSB_CHANNELS_MAX];

  UCHAR LfeElementTagSelect[PC_LFE_CHANNELS_MAX];

  UCHAR AssocDataElementTagSelect[PC_ASSOCDATA_MAX];

  UCHAR CcElementIsIndSw[PC_CCEL_MAX];
  UCHAR ValidCcElementTagSelect[PC_CCEL_MAX];

  UCHAR CommentFieldBytes;
  UCHAR Comment[PC_COMMENTLENGTH];
#endif /* TP_PCE_ENABLE */

  /* Helper variables for administration: */
  UCHAR isValid;               /*!< Flag showing if PCE has been read successfully. */
  UCHAR NumChannels;           /*!< Amount of audio channels summing all channel elements including LFEs */
  UCHAR NumEffectiveChannels;  /*!< Amount of audio channels summing only SCEs and CPEs */
  UCHAR elCounter;

} CProgramConfig;

typedef enum {
  ASCEXT_UNKOWN = -1,
  ASCEXT_SBR    = 0x2b7,
  ASCEXT_PS     = 0x548,
  ASCEXT_MPS    = 0x76a,
  ASCEXT_SAOC   = 0x7cb,
  ASCEXT_LDMPS  = 0x7cc

} TP_ASC_EXTENSION_ID;

#ifdef TP_GA_ENABLE
/**
 * GaSpecificConfig struct
 */
typedef struct {
  UINT m_frameLengthFlag ;
  UINT m_dependsOnCoreCoder ;
  UINT m_coreCoderDelay ;

  UINT m_extensionFlag ;
  UINT m_extensionFlag3 ;

  UINT m_layer;
  UINT m_numOfSubFrame;
  UINT m_layerLength;

} CSGaSpecificConfig;
#endif /* TP_GA_ENABLE */




#ifdef TP_ELD_ENABLE

typedef enum {
  ELDEXT_TERM  = 0x0,   /* Termination tag */
  ELDEXT_SAOC  = 0x1,   /* SAOC config */
  ELDEXT_LDSAC = 0x2    /* LD MPEG Surround config */
  /* reserved */
} ASC_ELD_EXT_TYPE;

typedef struct {
  UCHAR m_frameLengthFlag;

  UCHAR m_sbrPresentFlag;
  UCHAR m_useLdQmfTimeAlign;  /* Use LD-MPS QMF in SBR to achive time alignment */
  UCHAR m_sbrSamplingRate;
  UCHAR m_sbrCrcFlag;

} CSEldSpecificConfig;
#endif /* TP_ELD_ENABLE */




/**
 * Audio configuration struct, suitable for encoder and decoder configuration.
 */
typedef struct {

  /* XYZ Specific Data */
  union {
#ifdef TP_GA_ENABLE
    CSGaSpecificConfig    m_gaSpecificConfig;       /**< General audio specific configuration.         */
#endif /* TP_GA_ENABLE */
#ifdef TP_ELD_ENABLE
    CSEldSpecificConfig   m_eldSpecificConfig;      /**< ELD specific configuration.                   */
#endif /* TP_ELD_ENABLE */
  } m_sc;
  
  /* Common ASC parameters */
#ifdef TP_PCE_ENABLE
  CProgramConfig        m_progrConfigElement;     /**< Program configuration.                          */
#endif /* TP_PCE_ENABLE */

  AUDIO_OBJECT_TYPE     m_aot;                    /**< Audio Object Type.                              */
  UINT                  m_samplingFrequency;      /**< Samplerate.                                     */
  UINT                  m_samplesPerFrame;        /**< Amount of samples per frame.                    */
  UINT                  m_directMapping;          /**< Document this please !!                         */

  AUDIO_OBJECT_TYPE     m_extensionAudioObjectType;        /**< Audio object type                      */
  UINT                  m_extensionSamplingFrequency;      /**< Samplerate                             */  

  SCHAR                 m_channelConfiguration;   /**< Channel configuration index                     */

  SCHAR                 m_epConfig;               /**< Error protection index                           */
  SCHAR                 m_vcb11Flag;              /**< aacSectionDataResilienceFlag                     */
  SCHAR                 m_rvlcFlag;               /**< aacScalefactorDataResilienceFlag                 */
  SCHAR                 m_hcrFlag;                /**< aacSpectralDataResilienceFlag                    */

  SCHAR                 m_sbrPresentFlag;         /**< Flag indicating the presence of SBR data in the bitstream               */
  SCHAR                 m_psPresentFlag;          /**< Flag indicating the presence of parametric stereo data in the bitstream */
  UCHAR                 m_samplingFrequencyIndex; /**< Samplerate index                                 */
  UCHAR                 m_extensionSamplingFrequencyIndex; /**< Samplerate index                        */
  SCHAR                 m_extensionChannelConfiguration;   /**< Channel configuration index             */

} CSAudioSpecificConfig;

typedef INT (*cbUpdateConfig_t)(void*, const CSAudioSpecificConfig*);
typedef INT (*cbSsc_t)(
        void*, HANDLE_FDK_BITSTREAM,
        const AUDIO_OBJECT_TYPE coreCodec,
        const INT samplingFrequency,
        const INT muxMode,
        const INT configBytes
        );
typedef INT (*cbSbr_t)(
        void *                  self,
        HANDLE_FDK_BITSTREAM    hBs,
        const INT sampleRateIn,
        const INT sampleRateOut,
        const INT samplesPerFrame,
        const AUDIO_OBJECT_TYPE coreCodec,
        const MP4_ELEMENT_ID    elementID,
        const INT               elementIndex
        );

typedef struct  {
  cbUpdateConfig_t cbUpdateConfig; /*!< Function pointer for Config change notify callback.  */
  void *cbUpdateConfigData;        /*!< User data pointer for Config change notify callback. */
  cbSsc_t cbSsc;                   /*!< Function pointer for SSC parser callback. */
  void *cbSscData;                 /*!< User data pointer for SSC parser callback. */
  cbSbr_t cbSbr;                   /*!< Function pointer for SBR header parser callback. */
  void *cbSbrData;                 /*!< User data pointer for SBR header parser callback. */
} CSTpCallBacks;

static const UINT SamplingRateTable[] = 
{ 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0,
  0
};

static inline
int getSamplingRateIndex( UINT samplingRate )
{
  UINT sf_index, tableSize=sizeof(SamplingRateTable)/sizeof(UINT);

  for (sf_index=0; sf_index<tableSize; sf_index++) {
    if( SamplingRateTable[sf_index] == samplingRate ) break;
  }

  if (sf_index>tableSize-1) {
    return tableSize-1;
  }

  return sf_index;
}

/*
 * Get Channel count from channel configuration
 */
static inline int getNumberOfTotalChannels(int channelConfig)
{
  if (channelConfig > 0 && channelConfig < 8)
    return (channelConfig == 7)?8:channelConfig;
  else
    return 0;
}

static inline
int getNumberOfEffectiveChannels(const int channelConfig)
{
  const int n[] = {0,1,2,3,4,5,5,7};
  return n[channelConfig];
}

#endif /* __TP_DATA_H__ */
