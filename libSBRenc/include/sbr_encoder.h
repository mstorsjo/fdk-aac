/***************************  Fraunhofer IIS ***********************

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
   Author(s):
   Description: SBR encoder top level processing prototype

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef __SBR_ENCODER_H
#define __SBR_ENCODER_H

#include "common_fix.h"
#include "FDK_audio.h"

#include "FDK_bitstream.h"

/* core coder helpers */
#define MAX_TRANS_FAC         8
#define MAX_CODEC_FRAME_RATIO 2
#define MAX_PAYLOAD_SIZE    256

typedef struct
{
  INT bitRate;
  INT nChannels;
  INT sampleFreq;
  INT transFac;
  INT standardBitrate;
} CODEC_PARAM;

typedef enum
{
  SBR_MONO,
  SBR_LEFT_RIGHT,
  SBR_COUPLING,
  SBR_SWITCH_LRC
} SBR_STEREO_MODE;

/* bitstream syntax flags */
enum
{
  SBR_SYNTAX_LOW_DELAY = 0x0001,
  SBR_SYNTAX_SCALABLE  = 0x0002,
  SBR_SYNTAX_CRC       = 0x0004,
  SBR_SYNTAX_DRM_CRC   = 0x0008
};

typedef struct
{
  UINT             bitrateFrom;      /*!< inclusive */
  UINT             bitrateTo;        /*!< exclusive */

  USHORT           sampleRate;       /*!<   */
  UCHAR            numChannels;      /*!<   */

  UCHAR            startFreq;        /*!< bs_start_freq */
  UCHAR            startFreqSpeech;  /*!< bs_start_freq for speech config flag */
  UCHAR            stopFreq;         /*!< bs_stop_freq */
  UCHAR            stopFreqSpeech;   /*!< bs_stop_freq for speech config flag */

  UCHAR            numNoiseBands;    /*!<   */
  UCHAR            noiseFloorOffset; /*!<   */
  SCHAR            noiseMaxLevel;    /*!<   */
  SBR_STEREO_MODE  stereoMode;       /*!<   */
  UCHAR            freqScale;        /*!<   */
} sbrTuningTable_t;

typedef struct sbrConfiguration
{
  /*
     core coder dependent configurations
  */
  CODEC_PARAM codecSettings;  /*!< Core coder settings. To be set from core coder. */
  INT SendHeaderDataTime;     /*!< SBR header send update frequency in ms. */
  INT useWaveCoding;          /*!< Flag: usage of wavecoding tool. */
  INT crcSbr;                 /*!< Flag: usage of SBR-CRC. */
  INT dynBwSupported;         /*!< Flag: support for dynamic bandwidth in this combination. */
  INT parametricCoding;       /*!< Flag: usage of parametric coding tool. */
  int freq_res_fixfix[3];     /*!< Frequency resolution of envelopes in frame class FIXFIX
                                 0=1 Env; 1=2 Env; 2=4 Env; */
  /*
     core coder dependent tuning parameters
  */
  INT tran_thr;             /*!< SBR transient detector threshold (* 100). */
  INT noiseFloorOffset;     /*!< Noise floor offset.      */
  UINT useSpeechConfig;     /*!< Flag: adapt tuning parameters according to speech. */



  /*
     core coder independent configurations
  */
  INT sbrFrameSize;           /*!< SBR frame size in samples. Will be calculated from core coder settings. */
  INT sbr_data_extra;         /*!< Flag usage of data extra. */
  INT amp_res;                /*!< Amplitude resolution. */
  INT ana_max_level;          /*!< Noise insertion maximum level. */
  INT tran_fc;                /*!< Transient detector start frequency. */
  INT tran_det_mode;          /*!< Transient detector mode. */
  INT spread;                 /*!< Flag: usage of SBR spread. */
  INT stat;                   /*!< Flag: usage of static framing. */
  INT e;                      /*!< Number of envelopes when static framing is chosen. */
  SBR_STEREO_MODE stereoMode; /*!< SBR stereo mode. */
  INT deltaTAcrossFrames;     /*!< Flag: allow time-delta coding. */
  FIXP_DBL dF_edge_1stEnv;    /*!< Extra fraction delta-F coding is allowed to be more expensive. */
  FIXP_DBL dF_edge_incr;      /*!< Increment dF_edge_1stEnv this much if dT-coding was used this frame. */
  INT sbr_invf_mode;          /*!< Inverse filtering mode. */
  INT sbr_xpos_mode;          /*!< Transposer mode. */
  INT sbr_xpos_ctrl;          /*!< Transposer control. */
  INT sbr_xpos_level;         /*!< Transposer 3rd order level. */
  INT startFreq;              /*!< The start frequency table index. */
  INT stopFreq;               /*!< The stop frequency table index. */
  INT useSaPan;               /*!< Flag: usage of SAPAN stereo. */
  INT dynBwEnabled;           /*!< Flag: usage of dynamic bandwidth. */
  INT bParametricStereo;      /*!< Flag: usage of parametric stereo coding tool. */
  INT bDownSampledSbr;        /*!< Signal downsampled SBR is used. */

  /*
     header_extra1 configuration
  */
  UCHAR freqScale;            /*!< Frequency grouping. */
  INT alterScale;             /*!< Scale resolution. */
  INT sbr_noise_bands;        /*!< Number of noise bands. */


  /*
     header_extra2 configuration
  */
  INT sbr_limiter_bands;      /*!< Number of limiter bands. */
  INT sbr_limiter_gains;      /*!< Gain of limiter. */
  INT sbr_interpol_freq;      /*!< Flag: use interpolation in freq. direction. */
  INT sbr_smoothing_length;   /*!< Flag: choose length 4 or 0 (=on, off). */
  UCHAR init_amp_res_FF;
} sbrConfiguration, *sbrConfigurationPtr ;

typedef struct
{
  UINT sbrSyntaxFlags;                  /**< SBR syntax flags derived from AOT. */
  INT nChannels;                        /**< Number of channels.  */

  INT nSfb[2];                          /**< Number of SBR scalefactor bands for LO_RES and HI_RES (?) */
  INT num_Master;                       /**< Number of elements in v_k_master. */
  INT sampleFreq;                       /**< SBR sampling frequency. */
  INT frameSize;
  INT xOverFreq;                        /**< The SBR start frequency. */
  INT dynXOverFreq;                     /**< Used crossover frequency when dynamic bandwidth is enabled. */
  INT noQmfBands;                       /**< Number of QMF frequency bands. */
  INT noQmfSlots;                       /**< Number of QMF slots. */

  UCHAR *freqBandTable[2];              /**< Frequency table for low and hires, only MAX_FREQ_COEFFS/2 +1 coeefs actually needed for lowres. */
  UCHAR *v_k_master;                    /**< Master BandTable where freqBandTable is derived from. */


  SBR_STEREO_MODE stereoMode;
  INT noEnvChannels;                    /**< Number of envelope channels. */

  INT useWaveCoding;                    /**< Flag indicates whether to use wave coding at all.      */
  INT useParametricCoding;              /**< Flag indicates whether to use para coding at all.      */
  INT xposCtrlSwitch;                   /**< Flag indicates whether to switch xpos ctrl on the fly. */
  INT switchTransposers;                /**< Flag indicates whether to switch xpos on the fly .     */
  UCHAR initAmpResFF;
} SBR_CONFIG_DATA;

typedef SBR_CONFIG_DATA *HANDLE_SBR_CONFIG_DATA;

typedef struct {
  MP4_ELEMENT_ID elType;
  INT bitRate;
  int instanceTag;
  UCHAR fParametricStereo;
  UCHAR nChannelsInEl;
  UCHAR ChannelIndex[2];
} SBR_ELEMENT_INFO;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SBR_ENCODER *HANDLE_SBR_ENCODER;

/**
 * \brief  Get the max required input buffer size including delay balancing space
 *         for N audio channels.
 * \param noChannels  Number of audio channels.
 * \return            Max required input buffer size in bytes.
 */
INT sbrEncoder_GetInBufferSize(int noChannels);

INT sbrEncoder_Open(
        HANDLE_SBR_ENCODER  *phSbrEncoder,
        INT                  nElements,
        INT                  nChannels,
        INT                  supportPS
        );

/**
 * \brief get closest working bit rate to specified desired bit rate for a single SBR element
 * \param bitRate the desired target bit rate
 * \param numChannels the amount of audio channels
 * \param coreSampleRate the sample rate of the core coder
 * \param the current Audio Object Type
 * \return closest working bit rate to bitRate value
 */
UINT sbrEncoder_LimitBitRate(UINT bitRate, UINT numChannels, UINT coreSampleRate, AUDIO_OBJECT_TYPE aot);

/**
 * \brief                  Initialize SBR Encoder instance.
 * \param phSbrEncoder     Pointer to a SBR Encoder instance.
 * \param elInfo           Structure that describes the element/channel arrangement.
 * \param noElements       Amount of elements described in elInfo.
 * \param inputBuffer      Pointer to the encoder audio buffer
 * \param bandwidth        Returns the core audio encoder bandwidth (output)
 * \param bufferOffset     Returns the offset for the audio input data in order to do delay balancing.
 * \param numChannels      Input: Encoder input channels. output: core encoder channels.
 * \param sampleRate       Input: Encoder samplerate. output core encoder samplerate.
 * \param frameLength      Input: Encoder frameLength. output core encoder frameLength.
 * \param aot              Input: Desired AOT. output AOT to be used after parameter checking.
 * \param delay            Input: core encoder delay. Output: total delay because of SBR.
 * \param transformFactor  The core encoder transform factor (blockswitching).
 * \return                 0 on success, and non-zero if failed.
 */
INT sbrEncoder_Init( HANDLE_SBR_ENCODER hSbrEncoder,
                     SBR_ELEMENT_INFO elInfo[(6)],
                     int              noElements,
                     INT_PCM *inputBuffer,
                     INT   *bandwidth,
                     INT   *bufferOffset,
                     INT   *numChannels,
                     INT   *sampleRate,
                     INT   *frameLength,
                     AUDIO_OBJECT_TYPE *aot,
                     int *delay,
                     int  transformFactor,
                     ULONG statesInitFlag
                    );

/**
 * \brief             Do delay line buffers housekeeping. To be called after each encoded audio frame.
 * \param hEnvEnc     SBR Encoder handle.
 * \param timeBuffer  Pointer to the encoder audio buffer.
 * \return            0 on success, and non-zero if failed.
 */
INT sbrEncoder_UpdateBuffers(HANDLE_SBR_ENCODER hEnvEnc,
                             INT_PCM *timeBuffer
                            );

/**
 * \brief               Close SBR encoder instance.
 * \param phEbrEncoder  Handle of SBR encoder instance to be closed.
 * \return              void
 */
void sbrEncoder_Close(HANDLE_SBR_ENCODER *phEbrEncoder);

/**
 * \brief               Encode SBR data of one complete audio frame.
 * \param hEnvEncoder   Handle of SBR encoder instance.
 * \param samples       Time samples, always interleaved.
 * \param timeInStride  Channel stride factor of samples buffer.
 * \param sbrDataBits   Size of SBR payload in bits.
 * \param sbrData       SBR payload.
 * \return              0 on success, and non-zero if failed.
 */
INT sbrEncoder_EncodeFrame(HANDLE_SBR_ENCODER  hEnvEncoder,
                           INT_PCM            *samples,
                           UINT                timeInStride,
                           UINT                sbrDataBits[(6)],
                           UCHAR               sbrData[(6)][MAX_PAYLOAD_SIZE]
                          );

/**
 * \brief               Write SBR headers of one SBR element.
 * \param sbrEncoder    Handle of the SBR encoder instance.
 * \param hBs           Handle of bit stream handle to write SBR header to.
 * \param element_index Index of the SBR element which header should be written.
 * \param fSendHeaders  Flag indicating that the SBR encoder should send more headers in the SBR payload or not.
 * \return              void
 */
void sbrEncoder_GetHeader(SBR_ENCODER   *sbrEncoder,
                          HANDLE_FDK_BITSTREAM hBs,
                          INT            element_index,
                          int            fSendHeaders);

/**
 * \brief              SBR encoder bitrate estimation.
 * \param hSbrEncoder  SBR encoder handle.
 * \return             Estimated bitrate.
 */
INT sbrEncoder_GetEstimateBitrate(HANDLE_SBR_ENCODER hSbrEncoder);


/**
 * \brief              Delay between input data and downsampled output data.
 * \param hSbrEncoder  SBR encoder handle.
 * \return             Delay.
 */
INT sbrEncoder_GetInputDataDelay(HANDLE_SBR_ENCODER hSbrEncoder);

/**
 * \brief       Get decoder library version info.
 * \param info  Pointer to an allocated LIB_INFO struct, where library info is written to.
 * \return      0 on sucess.
 */
INT sbrEncoder_GetLibInfo(LIB_INFO *info);

void sbrPrintRAM(void);

void sbrPrintROM(void);

#ifdef __cplusplus
        }
#endif

#endif /* ifndef __SBR_MAIN_H */
