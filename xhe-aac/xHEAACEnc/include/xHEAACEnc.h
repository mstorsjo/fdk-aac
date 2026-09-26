
/* -----------------------------------------------------------------------------
Software Copyright License for The Fraunhofer FDK Extended High Efficiency AAC
Encoder Software for Android

© Copyright 1995 - 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1.    INTRODUCTION

The Fraunhofer FDK Extended High Efficiency AAC Encoder Software for Android
("FDK Extended High Efficiency AAC Encoder") is software that implements the
encoding of digital audio according to the MPEG-D Unified Speech and Audio
Coding (USAC) standard and MPEG-D Dynamic Range Control (DRC) standard. This FDK
Extended High Efficiency AAC Encoder Software is intended to be used on a wide
variety of Android devices. It is technically not suited to encode content for
digital radio broadcasting services, including DRM and similar standards.

Patent licenses for necessary patent claims for the FDK Extended High Efficiency
AAC Encoder Software (including those of Fraunhofer), for the use in commercial
products and services, may be obtained from the respective patent owners
individually and/or from Via Licensing Alliance (www.via-la.com).

Fraunhofer supports the development of Extended High Efficiency AAC products and
services by offering additional software, documentation, and technical advice.
In addition, it operates the xHE-AAC Trademark Program to ease interoperability
testing of end products. Please visit http://www.xhe-aac.com for more
information.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees, provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK Extended High
Efficiency AAC Encoder Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the
FDK Extended High Efficiency AAC Encoder Software and your modifications thereto
to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this software without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto.

Your modified versions of the FDK Extended High Efficiency AAC Encoder Software
must carry prominent notices stating that you changed the software and the date
of any change. For modified versions of the FDK Extended High Efficiency AAC
Encoder Software, the term "Fraunhofer FDK Extended High Efficiency AAC Encoder
Software for Android" must be replaced by the term "Third-Party Modified Version
of the Fraunhofer FDK Extended High Efficiency AAC Encoder Software for
Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty for patent non-infringement with respect to this
software. You may use this FDK Extended High Efficiency AAC Encoder Software or
modifications thereto only for purposes that are authorized by appropriate
patent licenses.

4.    DISCLAIMER

This FDK Extended High Efficiency AAC Encoder Software is provided by Fraunhofer
on behalf of the copyright holders and contributors "AS IS" and WITHOUT ANY
EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied
warranties of merchantability and fitness for a particular purpose. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if advised
of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - FDK Extended High Efficiency
AAC Encoder
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

#ifndef IIS_XHEAACENC_H
#define IIS_XHEAACENC_H

#ifdef __cplusplus
extern "C" {
#endif

#if (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L) || (defined __cplusplus && __cplusplus > 199711L) || defined _MSC_VER
#include "stdint.h"
#elif _WIN32
typedef unsigned __int64 uint64_t;
#else
typedef unsigned long long uint64_t;
#endif

#if defined(WIN32) || defined(WIN64)
#define IIS_XHEAACAPI __stdcall
#pragma pack(push, 1)
#else
#define IIS_XHEAACAPI
#endif

#define IIS_XHEAACENC_WARNING_FIRST (1000)           /**< First return code value for warnings */
#define IIS_XHEAACENC_ERROR_FIRST (100000)           /**< First return code value for general errors */
#define IIS_XHEAACENC_ERROR_PARAM_FIRST (200000)     /**< First return code value for configuration related errors */
#define IIS_XHEAACENC_ERROR_PARAMCOMB_FIRST (300000) /**< First return code value for parameter combination related errors */
#define IIS_XHEAACENC_DATA_FIRST (1)                 /**< First data identifier value for IIS_xHEAACEnc_SubmitData() */
#define IIS_XHEAACENC_PARAMETER_FIRST (10)           /**< First parameter identifier value for IIS_xHEAACEnc_SubmitData() */
#define IIS_XHEAACENC_MAX_ASC_SIZE (128)             /**< Maximum allowed size of the Audio Specific Config (ASC) in bytes */
#define IIS_XHEAACENC_MAX_WARNINGS (20)              /**< Maximum number of encoder warnings */
#define IIS_XHEAACENC_LIVE_PEAK_TO_LOUDNESS_RATIO (20)
#define IIS_XHEAACENC_LIVE_LOUDNESS_LEVEL_MIN (31)
#define IIS_XHEAACENC_DEFAULT_LIVE_SAMPLE_PEAK_MINIMUM (3)

/**********************************************************************/ /**
 Upon completion, all encoder functions return an error code. A return value
 of IIS_XHEAACENC_NO_ERROR confirms an error-free operation. Any other value
 indicates an error or warning, which can be distinguished as shown below.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_NO_ERROR = 0,                                                       /**< A given encoder function has executed successfully */
  IIS_XHEAACENC_WARNING_RAP_TOO_CLOSE = IIS_XHEAACENC_WARNING_FIRST,                /**< The distance between two RAPs is too close */
  IIS_XHEAACENC_WARNING_RAP_TOO_SOON,                                               /**< The call of the RAP is too late, it cannot be created any more */
  IIS_XHEAACENC_WARNING_INVALID_CONFIG,                                             /**< The given parameter list does not result in a valid encoder configuration */
  IIS_XHEAACENC_WARNING_CONFIG,                                                     /**< Warning(s) raised by encoder instance. Call IIS_xHEAACEnc_GetConfigWarnings() for details */
  IIS_XHEAACENC_WARNING_ENCODING,                                                   /**< Warning(s) raised by encoder instance. Call IIS_xHEAACEnc_GetEncodingWarnings() for details */
  IIS_XHEAACENC_ERROR_MEMORY = IIS_XHEAACENC_ERROR_FIRST,                           /**< Cannot allocate memory */
  IIS_XHEAACENC_ERROR_INITIALIZATION,                                               /**< The encoder could not be initialized with the given parameters */
  IIS_XHEAACENC_ERROR_RECONFIGURATION,                                              /**< The encoder could not be reconfigured with the given parameters */
  IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION,                                            /**< Error while memory allocation, probably no space left */
  IIS_XHEAACENC_ERROR_INSAMPLERATE_UNSUPPORTED,                                     /**< Unsupported input sampling rate */
  IIS_XHEAACENC_ERROR_COMB_INSAMPLERATE_UNSUPPORTED,                                /**< Unsupported input sampling rate for a sepcific configuration*/
  IIS_XHEAACENC_ERROR_INVALID_HANDLE,                                               /**< Internal error with an invalid handle */
  IIS_XHEAACENC_ERROR_INVALID_PARAM,                                                /**< Invalid function parameter, parameter type is not supported */
  IIS_XHEAACENC_ERROR_BUFFER_SIZE,                                                  /**< The provided output buffer is too small */
  IIS_XHEAACENC_ERROR_INVALID_RAP_POSITION,                                         /**< The position of the RAP is not possible (no frame border) */
  IIS_XHEAACENC_ERROR_WRONG_RAP_OCCURRENCE,                                         /**< The RAP Occurrence is not on demand, so no on demand request for a RAP possible */
  IIS_XHEAACENC_ERROR_SYNC_FRAME,                                                   /**< Internal error with the sync frame handling */
  IIS_XHEAACENC_ERROR_BITDISTRIBUTION,                                              /**< Internal error with bitdistribution */
  IIS_XHEAACENC_ERROR_PARAM_READ_ONLY,                                              /**< Parameter is read only and can not be set */
  IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT,                                          /**< Function parameter mismatch, parameter format is invalid */
  IIS_XHEAACENC_ERROR_INVALID_PARAMLENGTH,                                          /**< Function parameter length is invalid */
  IIS_XHEAACENC_ERROR_INVALID_VALUE,                                                /**< Invalid function parameter, parameter is out of range or invalid */
  IIS_XHEAACENC_ERROR_ITERATION,                                                    /**< Callback to the parameter list has failed */
  IIS_XHEAACENC_ERROR_INVALID_CONFIG,                                               /**< The given parameter list could not generate a valid encoder configuration */
  IIS_XHEAACENC_ERROR_CONFIG_NOT_FINALIZED,                                         /**< The configuration is not finalized */
  IIS_XHEAACENC_ERROR_CONFIG_ALREADY_FINALIZED,                                     /**< The configuration is already finalized */
  IIS_XHEAACENC_ERROR_TOO_MANY_INPUT_SAMPLES,                                       /**< More input samples provided than requested */
  IIS_XHEAACENC_ERROR_UNEXPECTED_INPUT_SAMPLES_FLUSHING,                            /**< Additional samples are fed to the encoder in flushing mode */
  IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT_TOO_MANY_INPUT_SAMPLES,                  /**< Loudness measurement has failed because of too many input samples */
  IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT_EXCEED_DEFAULT_LENGTH,                   /**< Number of provided input samples exceeds corresponding default input signal duration */
  IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT,                                         /**< Loudness measurement has failed */
  IIS_XHEAACENC_ERROR_LOUDNESS_MISMATCH,                                            /**< Externally provided loudness level deviates by more than 10 LU for USAC (AOT 42) */
  IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED,                                      /**< Exporting/importing the loudness data chunk failed */
  IIS_XHEAACENC_ERROR_LOUDNESS_MEASURED_OUT_OF_BOUND,                               /**< The measured loudness level of the input file is out of bound */
  IIS_XHEAACENC_ERROR_SAMPLE_PEAK_MEASURED_OUT_OF_BOUND,                            /**< The measured sample peak of the input file is out of bound */
  IIS_XHEAACENC_ERROR_SAMPLE_PEAK_MISMATCH,                                         /**< Externally provided sample peak value does not match the measured value */
  IIS_XHEAACENC_ERROR_SAMPLEPEAK_NO_LOUDNESS,                                       /**< Sample peak cannot be set without defining loudness level */
  IIS_XHEAACENC_ERROR_INVALID_LOUDNESS_LEVEL,                                       /**< Invalid loudness level for loudness processing */
  IIS_XHEAACENC_ERROR_SET_AND_MEASURED_LOUDNESS_TYPE_MISMATCH,                      /**< The type of the set loudness level does not match the type of the measured loudness */
  IIS_XHEAACENC_ERROR_LOUDNESS_PROCESS,                                             /**< Internal loudness processing error */
  IIS_XHEAACENC_ERROR_LOUDNESS_PROCESS_SAMPLES_TOO_FEW,                             /**< Too few samples processed for loudness calculation */
  IIS_XHEAACENC_ERROR_LOUDNESS_IMPORT_CONFIG_MISMATCH,                              /**< The imported loudness config differs from the expected config */
  IIS_XHEAACENC_ERROR_LOUDNESS_IMPORT_MISMATCH,                                     /**< The imported loudness differs from the expected loudness */
  IIS_XHEAACENC_ERROR_LOUDNESS_ALREADY_INITIALIZED,                                 /**< The loudness handle already was initialized */
  IIS_XHEAACENC_ERROR_UNKNOWN,                                                      /**< Unknown or undocumented error */
  IIS_XHEAACENC_ERROR_INTERNAL,                                                     /**< Internal error */
  IIS_XHEAACENC_ERROR_PARAM_AOT = IIS_XHEAACENC_ERROR_PARAM_FIRST,                  /**< Invalid or missing parameter for AOT */
  IIS_XHEAACENC_ERROR_PARAM_BITRATEMODE,                                            /**< Invalid or missing parameter for bitrate mode (constant bitrate or different variable bitrate modes) */
  IIS_XHEAACENC_ERROR_PARAM_BITRATE,                                                /**< Invalid or missing parameter for bitrate */
  IIS_XHEAACENC_ERROR_PARAM_OPERATING_MODE,                                         /**< Invalid or missing parameter for operating mode */
  IIS_XHEAACENC_ERROR_PARAM_PRESET,                                                 /**< Invalid or missing parameter for preset */
  IIS_XHEAACENC_ERROR_PARAM_INSAMPLERATE,                                           /**< Invalid or missing parameter for input sampling rate */
  IIS_XHEAACENC_ERROR_PARAM_OUTSAMPLERATE,                                          /**< Invalid or missing parameter for output sampling rate */
  IIS_XHEAACENC_ERROR_PARAM_CHANNELCONFIG,                                          /**< Invalid or missing parameter for channel config */
  IIS_XHEAACENC_ERROR_PARAM_TRANSPORTFORMAT,                                        /**< Invalid or missing parameter for transport format */
  IIS_XHEAACENC_ERROR_PARAM_RAP_INTERVAL,                                           /**< Invalid or missing parameter for random access interval */
  IIS_XHEAACENC_ERROR_PARAM_RAP_OCCURRENCE,                                         /**< Invalid or missing parameter for random access point occurrence */
  IIS_XHEAACENC_ERROR_PARAM_MAX_IF_DISTANCE,                                        /**< Invalid or missing parameter for maximum independency frame distance */
  IIS_XHEAACENC_ERROR_PARAM_MAX_IF_DISTANCE_DRM,                                    /**< Invalid or missing parameter for maximum independency frame distance for DRM (has to be equivalent to 1 frame) */
  IIS_XHEAACENC_ERROR_PARAM_STREAMID,                                               /**< Invalid or missing parameter for stream ID */
  IIS_XHEAACENC_ERROR_PARAM_LOUDNESS_LEVEL,                                         /**< Invalid or missing parameter for loudness level */
  IIS_XHEAACENC_ERROR_PARAM_ALBUM_LOUDNESS_LEVEL,                                   /**< Invalid or missing parameter for album loudness level */
  IIS_XHEAACENC_ERROR_PARAM_ANCHOR_LOUDNESS_LEVEL,                                  /**< Invalid or missing parameter for anchor loudness level */
  IIS_XHEAACENC_ERROR_PARAM_NO_LOUDNESS_PROVIDED,                                   /**< No loudness value has been provided. Loudness settings require a valid loudness value (e.g. loudness level or anchor loudness level) */
  IIS_XHEAACENC_ERROR_PARAM_LOUDNESS_DATA,                                          /**< Invalid or missing parameter for loudness data */
  IIS_XHEAACENC_ERROR_PARAM_SAMPLE_PEAK,                                            /**< Invalid or missing parameter for sample peak */
  IIS_XHEAACENC_ERROR_PARAM_QUIET_LOUDNESS_THRESHOLD,                               /**< Invalid parameter for quiet loudness threshold */
  IIS_XHEAACENC_ERROR_PARAM_TARGET_LOUDNESS_RANGE,                                  /**< Invalid or missing parameter for target loudness range: The DRC target loudness range is out of range. It shall be in the range [6.0, 16.0].*/
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_PROG_REF_LEVEL,                                   /**< Invalid or missing parameter for MPEG-4 program reference level */
  IIS_XHEAACENC_ERROR_PARAM_DRCMODE,                                                /**< Invalid or missing parameter for DRC mode */
  IIS_XHEAACENC_ERROR_PARAM_DRCCHARACTERISTIC,                                      /**< Invalid or missing parameter for DRC characteristic */
  IIS_XHEAACENC_ERROR_PARAM_LIVELOUDNESSLEVEL,                                      /**< Invalid or missing parameter for Live loudness level */
  IIS_XHEAACENC_ERROR_PARAM_LIVESAMPLEPEAK,                                         /**< Invalid or missing parameter for Live sample peak */
  IIS_XHEAACENC_ERROR_PARAM_LIVEMODE,                                               /**< Invalid or missing parameter for Live mode */
  IIS_XHEAACENC_ERROR_PARAM_LIVERELMAXGAIN,                                         /**< Invalid value for live loudness relative max gain. Allowed values are [30,45]dB */
  IIS_XHEAACENC_ERROR_PARAM_FRAMESAMPLES,                                           /**< Invalid or missing parameter for frameSamples */
  IIS_XHEAACENC_ERROR_PARAM_FLUSHINGMODE,                                           /**< Invalid or missing parameter for flushing mode */
  IIS_XHEAACENC_ERROR_PARAM_MPEG2AAC,                                               /**< Invalid or missing parameter for Legacy mpeg2 AAC mode */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_DRC_LIGHT_PROF,                                   /**< Invalid or missing parameter for Light Profile */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_DRC_HEAVY_PROF,                                   /**< Invalid or missing parameter for Heavy Profile */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_DRC_LIGHT_TARGET_LEVEL,                           /**< Invalid or missing parameter for Light Target Level */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_DRC_HEAVY_TARGET_LEVEL,                           /**< Invalid or missing parameter for Heavy Target Level */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_ANCILLARY_DATA,                                   /**< Invalid or missing parameter for Ancillary Data */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_CENTER_MIX_LEVEL,                                 /**< Invalid or missing parameter for Center Mix Level */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_SURROUND_MIX_LEVEL,                               /**< Invalid or missing parameter for Surround Mix Level */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_WRITE_PCE_MIXDOWN_IDX,                            /**< Invalid or missing parameter for PCE Downmix Index */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_ETSI_DWNMIX_PRESENT,                              /**< Invalid or missing parameter for ETSI Downmix Present */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_DOLBY_SURROUND_MODE,                              /**< Invalid or missing parameter for Dolby Surround Mode */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_DRC_PRES_MODE,                                    /**< Invalid or missing parameter for DRC Presentation Mode */
  IIS_XHEAACENC_ERROR_PARAM_MPEG4_METADATA_MODE,                                    /**< Invalid or missing parameter for Metadata Mode */
  IIS_XHEAACENC_ERROR_PARAM_NO_START_STOP,                                          /**< Invalid or missing parameter for block switching no start stop */
  IIS_XHEAACENC_ERROR_PARAM_UNSUPPORTED,                                            /**< The parameter is not supported */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG2AAC_AOT = IIS_XHEAACENC_ERROR_PARAMCOMB_FIRST, /**< Invalid parameter combination: Legacy mpeg2 AAC mode only supported for AOTs : 2, 5, 29 */
  IIS_XHEAACENC_ERROR_PARAMCOMB_NOSTARTSTOP_AOT,                                    /**< Invalid parameter combination: disabling start stop sequence only supported for AOTs : 2, 5, 29 */
  IIS_XHEAACENC_ERROR_PARAMCOMB_BASE,                                               /**< Invalid parameter combination: The combination of AOT, Operating Mode, Bitrate/BitrateMode, ChannelConfig and Transportformat is not allowed */
  IIS_XHEAACENC_ERROR_PARAMCOMB_BITRATE_CORESR,                                     /**< Invalid parameter combination: This bitrate is not allowed for the used core samplerate */
  IIS_XHEAACENC_ERROR_PARAMCOMB_BITRATE_DRM,                                        /**< Invalid parameter combination: This bitrate is not allowed for DRM (must be a multiple of 20) */
  IIS_XHEAACENC_ERROR_PARAMCOMB_SR_NOSBR_DRM30,                                     /**< Invalid parameter combination: SBR must be enabled for drm30 with output samplerates of 38400 Hz and 48000 Hz */
  IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_LOUDNESS,                                       /**< Invalid parameter combination: If Dynamic Range Control (DRC) is enabled, a Loudness Level is required too */
  IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_MODE_LRACONTROL_DATA,                           /**< Invalid parameter combination: If DRC mode is set, LRA Control data (two-pass implementation) shall be supplied. */
  IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_MODE_TARGET_LOUDNESS_RANGE,                     /**< invalid parameter combination: If target loudness range is set, DRC mode shall also be set */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LOUDNESSLEVEL_DISABLELOUDNESS,                      /**< Invalid parameter combination: If disable loudness is set, no Loudness Level is allowed */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LOUDNESSLEVEL_ALBUMLOUDNESS,                        /**< Invalid parameter combination: Album loudness cannot be set without setting program loudnessLevel */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LOUDNESSLEVEL_ANCHORLOUDNESS,                       /**< Invalid parameter combination: Usage of both program loudness and anchor loudness is not allowed. Use only one of the two levels */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LOUDNESSLEVEL_MPEG4PROGREFLEVEL,                    /**< Invalid parameter combination: Usage of both loudness level and mpeg4 prog ref level is not allowed. Use only one the two levels */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD,               /**< Invalid parameter combination: Loudness level and quiet loudness threshold cannot be set simultaneously */
  IIS_XHEAACENC_ERROR_PARAMCOMB_ANCHORLOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD,         /**< Invalid parameter combination: Anchor loudness level and quiet loudness threshold cannot be set simultaneously */
  IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_LOUDNESSLEVEL,                                  /**< Invalid parameter combination: Usage of legacy AOT with loudnessLevel is not allowed.*/
  IIS_XHEAACENC_ERROR_PARAMCOMB_PROGREFLEVEL_MPEGDDRC,                              /**< Invalid parameter combination: Usage of program reference level and MPEG-D DRC is not allowed. ProgRefLevel is only allowed with MPEG-4 DRC */
  IIS_XHEAACENC_ERROR_PARAMCOMB_PROGREFLEVEl_AOT,                                   /**< Invalid parameter combination: Usage of AOT 42 with program reference level is not allowed*/
  IIS_XHEAACENC_ERROR_PARAMCOMB_PROGREFLEVEL_QUIETLOUDNESSTHRESHOLD,                /**< Invalid parameter combination: Usage of quiet loudness threshold with program reference level is not allowed*/
  IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4PROGREFLEVEL_MPEG4DRC,                         /**< Invalid parameter combination: MPEG-4 program reference level is required for MPEG-4 Metadata */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4DRC_FOR_MPEG4METADATA,                         /**< Invalid parameter combination: MPEG-4 DRC light profile must be present for this metadata mode */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4DRC_DISABLELOUDNESSMEASUREMENT,                /**< Invalid parameter combination: MPEG-4 DRC and disable loudness measurement */
  IIS_XHEAACENC_ERROR_PARAMCOMB_CHANNELCONFIG_LIVELOUDNESS,                         /**< Invalid parameter combination: Live loudness leveling only supports mono and stereo input channel format */
  IIS_XHEAACENC_ERROR_PARAMCOMB_SAMPLEPEAK_LIVESAMPLEPEAK,                          /**< Invalid parameter combination: Live sample peakand sample peak cannot be set simultaneously */
  IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_LIVELOUDNESSLEVEL,                              /**< Invalid parameter combination: Live loudness level is only allowed for USAC (AOT 42) */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LIVESAMPLEPEAK_NOLIVELOUDNESSLEVEL,                 /**< Invalid parameter combination: Live sample peak cannot be set without setting live loudness level */
  IIS_XHEAACENC_ERROR_PARAMCOMB_SAMPLEPEAK_LIVELOUDNESSLEVEL,                       /**< Invalid parameter combination: Live loudness level cannot be set with regular sample peak, please use live sample peak */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_DISABLELOUDNESS,                  /**< Invalid parameter combination: If disable loudness is set, no Live loudness Level is allowed */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_LOUDNESSLEVEL,                    /**< Invalid parameter combination: Live loudness level and loudness level cannot be set simultaneously */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_ALBUMLOUDNESS,                    /**< Invalid parameter combination: Live loudness level and album loudness cannot be set simultaneously */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_ANCHORLOUDNESS,                   /**< Invalid parameter combination: Live loudness level and anchor loudness cannot be set simultaneously */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD,           /**< Invalid parameter combination: Live loudness level and quiet loudness threshold cannot be set simultaneously */
  IIS_XHEAACENC_ERROR_PARAMCOMB_NO_LIVELOUDNESSLEVEL_LIVEMODE,                      /**< Invalid parameter combination: Live loudness level mode cannot be set without setting live loudness level */
  IIS_XHEAACENC_ERROR_PARAMCOMB_NO_LIVELOUDNESSLEVEL_LIVELOUDNESSRELMAXGAIN,        /**< Invalid parameter combination: Live loudness relative maximum gain cannot be set without setting live loudness level */
  IIS_XHEAACENC_ERROR_PARAMCOMB_LOW_LIVELOUDNESSLEVEL_LIVESAMPLEPEAK,               /**< Invalid parameter combination: Live sample peak too low for selected live loudness level */
  IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_MODE_LIVE_LRA_CONTROL,                          /**< Invalid parameter combination: Live loudness range control only supports DRC mode PARAMLIST_DRCMODE_LN_NE_LI_LRACONTROL_GE_FILMSTD or PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL*/
  IIS_XHEAACENC_ERROR_PARAMCOMB_RAP_TRANSPORTFORMAT,                                /**< Invalid parameter combination: The RAP property / occurrence must not be off for LATM / LATMLOAS / ADTS*/
  IIS_XHEAACENC_ERROR_PARAMCOMB_RAP_OCCURRENCE_RAP_INTERVAL,                        /**< Invalid parameter combination: The RAP occurrence CONSTANT_INTERVAL requires a valid RAP interval. Also, if the RAP interval is given, the RAP occurrence must be CONSTANT_INTERVAL. */
  IIS_XHEAACENC_ERROR_PARAMCOMB_RAP_INTERVAL_RAP_MIN_INTERVAL,                      /**< Invalid parameter combination: The provided RAP interval is smaller than the minimum allowed RAP interval */
  IIS_XHEAACENC_ERROR_PARAMCOMB_SHORTEST_RAP_INTVL_RAP_OCCURRENCE,                  /**< Invalid parameter combination: shortest RAP interval is only allowed for rap occurrence "rap on demand" */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MD_MPEG_DRC_HEAVY,                                  /**< Invalid parameter combination: MPEG-4 DRC Profile Heavy is not allowed for MPEG Metadata Mode */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MD_MODE_DRC_PRES_MODE,                              /**< Invalid parameter combination: MPEG-4 DRC Presentation Mode is not allowed for this Metadata Mode */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MD_CHANNELCONFIG_DOWNMIX,                           /**< Invalid parameter combination: channel config and MPEG-4 metadata downmix indication */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MD_DRC_PRES_TARGETREFLVL,                           /**< Invalid parameter combination: specified target reference level value is not allowed for this MPEG-4 DRC presentation mode */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MD_MODE_CHANNELCONFIG_DOWNMIX,                      /**< Invalid parameter combination: metadata mode, channel config and downmix indication */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MD_MODE_SURROUND,                                   /**< Invalid parameter combination: metadata mode and MPEG-4 metadata surround indication */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MD_CHANNELCONFIG_SURROUND,                          /**< Invalid parameter combination: channel config and MPEG-4 metadata surround indication */
  IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_DRM,                                            /**< Invalid parameter combination: DRC is not allowed for drm30 or drmPlus */
  IIS_XHEAACENC_ERROR_PARAMCOMB_STREAMID_AOT,                                       /**< Invalid parameter combination: Stream ID is not allowed to be set for non-usac */
  IIS_XHEAACENC_ERROR_PARAMCOMB_FLUSHMODE_DRC,                                      /**< Invalid parameter combination: flushingMode synchronized is only allowed if DRC is not active */
  IIS_XHEAACENC_ERROR_PARAMCOMB_PRESET_AOT,                                         /**< Invalid parameter combination: Preset settings for this AOT is not supported */
  IIS_XHEAACENC_ERROR_PARAMCOMB_PRESET_AOT_CHANNELCONFIG,                           /**< Invalid parameter combination: The chosen preset is not available for this channel config*/
  IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4DRC_LOUDNESS,                                  /**< Invalid parameter combination: If MPEG-4 DRC is enabled, a Loudness Level is required too */
  IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_MPEG4DRC,                                       /**< Invalid parameter combination: MPEG-4 DRC is not allowed for USAC (AOT 42) */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MDMODE_PROGREFLVL,                                  /**< Invalid parameter combination: This metadata mode requires a program reference level */
  IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_MD,                                             /**< Invalid parameter combination: Extended Metadata Support is not allowed for USAC (AOT 42) */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MD_NONE_DRC,                                        /**< Invalid parameter combination: MPEG-4 DRC cannot be set with Metadata Mode None */
  IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_DRC_MODE,                                       /**< Invalid parameter combination: The chosen DRC mode is not allowed for non-USAC object types (AOT 2, 5, 29) */
  IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_MPEG4DRC,                                       /**< Invalid parameter combination: MPEG-4 DRC cannot be combined with MPEG-D DRC */
  IIS_XHEAACENC_ERROR_PARAMCOMB_VBR_MPEG4DRC,                                       /**< Invalid parameter combination: MPEG-4 DRC cannot be combined with VBR modes */
  IIS_XHEAACENC_ERROR_PARAMCOMB_TARGETLOUDNESS_FOR_MPEG4DRCLIGHT,                   /**< Invalid parameter combination: No target loudness set for MPEG-4 DRC Profile Light */
  IIS_XHEAACENC_ERROR_PARAMCOMB_TARGETLOUDNESS_FOR_MPEG4DRCHEAVY,                   /**< Invalid parameter combination: No target loudness set for MPEG-4 DRC Profile Heavy */
  IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4DRCHEAVY_NO_MPEG4DRCLIGHT,                     /**< Invalid parameter combination: MPEG-4 DRC Profile Heavy is chosen without MPEG4-DRC Light */
  IIS_XHEAACENC_ERROR_PARAMCOMB_BITRATE_BITRATEMODE,                                /**< Invalid parameter combination: Bitrate and BitrateMode does not fit */
  IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_LOUDNESS_DRC,                                   /**< Invalid parameter combination: Loudness is not allowed for (HE-)AAC without DRC */
  IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_BITRATEMODE_CHANNELCONFIG,                      /**< Invalid parameter combination: This AOT is not allowed for this bitrate mode and channel config */
  IIS_XHEAACENC_ERROR_PARAMCOMB_OPERATINGMODE_PRIMING,                              /**< Invalid parameter combination: AOT 42 is only allowed with priming mode none */
  IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_42_OPERATINGMODE_PRIMING_MODE,                  /**< Invalid parameter combination: Priming full is only allowed with operating mode general */
  IIS_XHEAACENC_ERROR_PARAMCOMB_STREAMID_SWITCHABLE_AOT,                            /**< Invalid parameter combination: AOT 42 and operating mode switchable requires a stream ID */
  IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_SBRSIGNALING,                                   /**< Invalid parameter combination: SBR signaling is not applicable for USAC (AOT 42) and AAC-LC (AOT 2) */
} IIS_XHEAACENC_RETURN_CODE;

/**********************************************************************/ /**
 During executions, all encoder functions may raise a warning. A warning is
 not fatal for the execution of the encoder instance, but it may require
 attention of the user to ensure desired behaviour.
 A call to the function IIS_xHEAACEnc_GetEncodingWarnings()/
 IIS_xHEAACEnc_GetConfigWarnings() returns an array with
 all the warnings that have been raised since the last call to this function.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_WARNING_INVALID = 0,
  IIS_XHEAACENC_WARN_RAP_TOO_CLOSE = IIS_XHEAACENC_WARNING_FIRST, /**< The distance between two RAPs is too close */
  IIS_XHEAACENC_WARN_RAP_TOO_SOON,                                /**< The call of the RAP is too late, it cannot be created any more */
  IIS_XHEAACENC_WARN_INVALID_CONFIG,                              /**< The given parameter list does not result in a valid encoder configuration */
  IIS_XHEAACENC_WARN_LOUDNESS_DEVIATION,                          /**< User provided loudness level deviates between 2-10 LUFS as compared against internal calculations */
  IIS_XHEAACENC_WARN_LARGE_LOUDNESS_DEVIATION,                    /**< User provided loudness level deviates by more than 10 LUFS as compared against internal calculations */
  IIS_XHEAACENC_WARN_SAMPLE_PEAK_DEVIATION,                       /**< User provided sample peak value deviates by more than 0.5dB as compared against internal calculations */
  IIS_XHEAACENC_WARN_TOO_FEW_SAMPLES_TO_VALIDATE_LOUDNESS,        /**< User has provided too few samples to validate the loudness level internally*/
  IIS_XHEAACENC_WARN_MORE_SAMPLES_EXPECTED,                       /**< User has provided too few samples as compared to the imported loudness data*/
  IIS_XHEAACENC_WARN_MPEG4_MD_INVALID_VALUE,                      /**< MPEG4 metadata param value out of range: corrected to closet in-range value*/
  IIS_XHEAACENC_WARN_MPEG4_MD_UPDATE_NOT_ALLOWED,                 /**< MPEG4 metadata param value out of range: updating param value ist not allowed*/
  IIS_XHEAACENC_WARN_DEPRECATED_SETTING_ADVANCED_KEY,             /**< The way of unlocking advanced feature sets with an advanced key/value pair is deprecated. Refer to the API documentation for more information*/
  IIS_XHEAACENC_WARN_QUIET_MEASURED_LOUDNESS,                     /**< Measured loudness level is below the quiet loudness threshold */
  IIS_XHEAACENC_WARN_ASC_REQUEST_WITH_IMPLICIT_SBRSIGNALING,      /**< ACS request with implicit SBR signaling --> The presence of SBR data will not be signaled in the ASC */
  IIS_XHEAACENC_WARNING_LAST = IIS_XHEAACENC_WARNING_FIRST +
                               IIS_XHEAACENC_MAX_WARNINGS
} IIS_XHEAACENC_WARNING;

/**********************************************************************/ /**
 The audio object types (AOT's) available in this library are listed below.
 The AOT indicates the encoding tools/modules initialized in a given encoder
 configuration. Please see ISO/IEC 23003-3 for the full set of encoding
 tools/modules available based on the AOT selected. Each AOT focuses on a
 different encoding priority i.e. encoding speed, encoding complexity or
 audio fidelity.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_AOT_INVALID = -1, /**< The audio object type is not supported or does not exist */
  IIS_XHEAACENC_AOT_USAC = 42     /**< Unified Speech and Audio Coding (USAC) */
} IIS_XHEAACENC_AOT;

/**********************************************************************/ /**
 The bit rate mode indicates whether constant bit rate (CBR) or variable bit
 rate (VBR) encoding should be performed. CBR coding enforces a certain
 constant bit consumption independent of the input signal, whereas VBR coding
 adapts the bit consumption to the psychoacoustic requirements of the signal.

 In case of CBR, the encoder uses the specified audio object type (AOT 42)
 and specified bit rate.
 In case of VBR modes, the encoder uses the specified audio object type (AOT 42)
 and selects a pre-defined bit rate.

 Configurations (mono):
   - VBR0 can use AOT 42 (USAC) at around  20 kbit/s                 \note VBR0 mono is an experimental feature. Experience may change in future releases.
   - VBR1 can use AOT 42 (USAC) at around  32 kbit/s                 \note VBR1 mono is an experimental feature. Experience may change in future releases.
   - VBR2 can use AOT 42 (USAC) at around  40 kbit/s                 \note VBR2 mono is an experimental feature. Experience may change in future releases.
   - VBR3 can use AOT 42 (USAC) at around  56 kbit/s
   - VBR4 can use AOT 42 (USAC) at around  72 kbit/s
   - VBR5 can use AOT 42 (USAC) at around 104 kbit/s
   - VBR6 can use AOT 42 (USAC) at around 136 kbit/s

 Configurations (stereo):
   - VBR0 can use AOT 42 (USAC) at around  24 kbit/s                 \note VBR0 stereo is an experimental feature. Experience may change in future releases.
   - VBR1 can use AOT 42 (USAC) at around  40 kbit/s
   - VBR2 can use AOT 42 (USAC) at around  64 kbit/s
   - VBR3 can use AOT 42 (USAC) at around  96 kbit/s
   - VBR4 can use AOT 42 (USAC) at around 128 kbit/s
   - VBR5 can use AOT 42 (USAC) at around 192 kbit/s
   - VBR6 can use AOT 42 (USAC) at around 256 kbit/s
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_BITRATEMODE_INVALID = -1, /**< The bitrate mode is not supported or does not exist */
  IIS_XHEAACENC_BITRATEMODE_CBR = 0,      /**< Constant bitrate mode */
  IIS_XHEAACENC_BITRATEMODE_AAC_VBR1 = 1, /**< Variable bitrate mode 1 */
  IIS_XHEAACENC_BITRATEMODE_AAC_VBR2 = 2, /**< Variable bitrate mode 2 */
  IIS_XHEAACENC_BITRATEMODE_AAC_VBR3 = 3, /**< Variable bitrate mode 3 */
  IIS_XHEAACENC_BITRATEMODE_AAC_VBR4 = 4, /**< Variable bitrate mode 4 */
  IIS_XHEAACENC_BITRATEMODE_AAC_VBR5 = 5, /**< Variable bitrate mode 5 */
  IIS_XHEAACENC_BITRATEMODE_AAC_VBR6 = 6, /**< Variable bitrate mode 6 */
  IIS_XHEAACENC_BITRATEMODE_AAC_VBR0 = 7  /**< Variable bitrate mode 0 */
} IIS_XHEAACENC_BITRATEMODE;

/**********************************************************************/ /**
 For a given encoder configuration the internal sampling rate can be set
 using one of the sampling rates listed below.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_OUTSAMPLERATE_INVALID = -1,  /**< Invalid sampling rate */
  IIS_XHEAACENC_OUTSAMPLERATE_MIN = 7350,    /**<  7350 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_7350 = 7350,   /**<  7350 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_8000 = 8000,   /**<  8000 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_9600 = 9600,   /**<  9600 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_11025 = 11025, /**< 11025 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_12000 = 12000, /**< 12000 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_16000 = 16000, /**< 16000 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_19200 = 19200, /**< 19200 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_22050 = 22050, /**< 22050 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_24000 = 24000, /**< 24000 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_29400 = 29400, /**< 29400 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_32000 = 32000, /**< 32000 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_35280 = 35280, /**< 35280 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_38400 = 38400, /**< 38400 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_44100 = 44100, /**< 44100 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_48000 = 48000, /**< 48000 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_64000 = 64000, /**< 64000 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_88200 = 88200, /**< 88200 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_96000 = 96000, /**< 96000 Hz */
  IIS_XHEAACENC_OUTSAMPLERATE_MAX = 96000    /**< 96000 Hz */
} IIS_XHEAACENC_OUTSAMPLERATE;

/**********************************************************************/ /**
 The channel configurations unambiguously define the number of channels,
 channel elements and associated loudspeaker mapping. For each channel
 contained in the bit stream there is an associated loudspeaker position to
 which this particular channel shall be mapped.
 The input channel ordering follows the RIFF WAVE channel ordering,
 the internal processing as well as the reproduced audio follows the
 MPEG-CICP channel configuration, see ISO/IEC 23091-3. Fore more details
 it is refereed to \ref channel_mapping.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_CHANNELCONFIG_INVALID = -1, /**< Invalid channel configuration */
  IIS_XHEAACENC_CHANNELCONFIG_MONO = 1,     /**< 1 channel audio, reproduced to MPEG CICP 1 */
  IIS_XHEAACENC_CHANNELCONFIG_STEREO = 2,   /**< 2 channel audio, reproduced to MPEG CICP 2 */
} IIS_XHEAACENC_CHANNELCONFIG;

/**********************************************************************/ /**
 The bit stream transport formats available in this library determine how the
 audio data and meta data is organized (multiplexed) in the file. The transport
 format should be selected based on the intended use case of the bit stream.

 For example:
   - Storage bit streams:      MP4
   - Multiplex bit streams:    LATM/LOAS
   - Transmission bit streams: ADTS and LATM/LOAS

 For more details on transport/file formats please see ISO/IEC 14496-12.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_TRANSPORTFORMAT_INVALID = -1, /**< Invalid transport format */
  IIS_XHEAACENC_TRANSPORTFORMAT_RAW = 0,      /**< "raw" (plain access units) */
  IIS_XHEAACENC_TRANSPORTFORMAT_ADTS = 2,     /**< Audio Data Transport Stream (ADTS) format. See ISO/IEC 13818-7 and 14496-3 for full details. */
  IIS_XHEAACENC_TRANSPORTFORMAT_LATMLOAS = 4  /**< Low Overhead Audio Transport Multiplex (LATM). Low Overhead Audio Stream (LOAS) transport format (audio only). See ISO/IEC 14496-3 for full details. */
} IIS_XHEAACENC_TRANSPORTFORMAT;

/**********************************************************************/ /**
 The number of audio samples per access unit, i.e. audio frame
 Note: not all frame sample sizes are supported in this variant of the encoder.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_FRAMESAMPLES_INVALID = -1, /**< Invalid samples per frame */
  IIS_XHEAACENC_FRAMESAMPLES_768 = 768,    /**< 768 samples per frame */
  IIS_XHEAACENC_FRAMESAMPLES_960 = 960,    /**< 960 samples per frame */
  IIS_XHEAACENC_FRAMESAMPLES_1024 = 1024,  /**< 1024 samples per frame */
  IIS_XHEAACENC_FRAMESAMPLES_1920 = 1920,  /**< 1920 samples per frame */
  IIS_XHEAACENC_FRAMESAMPLES_2048 = 2048,  /**< 2048 samples per frame */
  IIS_XHEAACENC_FRAMESAMPLES_4096 = 4096   /**< 4096 samples per frame */
} IIS_XHEAACENC_FRAMESAMPLES;

/**********************************************************************/ /**
 The sync frame type indicates whether an access unit (AU) contains all
 information necessary for the decoder to begin decoding a bit stream.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_SYNCFRAME_INVALID = -1,                            /**< Invalid sync frame type */
  IIS_XHEAACENC_SYNCFRAME_NO = 0,                                  /**< Regular frame, e.g. No sync */
  IIS_XHEAACENC_SYNCFRAME_INDEPENDENT_FRAME = 2,                   /**< USAC independently decodable frame (IF) */
  IIS_XHEAACENC_SYNCFRAME_IMMEDIATE_PLAY_OUT_FRAME = 3,            /**< USAC immediate playout frame (IPF) with configuration information */
  IIS_XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_IF = 4,                /**< Independently decodable frame which contains an LATM StreamMuxConfig */
  IIS_XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_IPF = 5,               /**< USAC immediate playout frame (IPF) with configuration information and an LATM StreamMuxConfig */
  IIS_XHEAACENC_SYNCFRAME_HE_AAC_RAP_SWITCHABLE = 6,               /**< (HE-)AAC frame which allows seamless switching between streams */
  IIS_XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_SWITCHABLE = 8, /**< (HE-)AAC frame which allows seamless switching between streams and which contains an LATM StreamMuxConfig */
  IIS_XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_ACCESS = 9      /**< (HE-)AAC frame which contains an LATM StreamMuxConfig */
} IIS_XHEAACENC_SYNCFRAME_TYPES;

/**********************************************************************/ /**
 The random access point occurrence defines the frequency of occurrence of
 access points in the bit stream.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_RAP_OCCURRENCE_INVALID = -1,          /**< Invalid RAP occurrence */
  IIS_XHEAACENC_RAP_OCCURRENCE_CONSTANT_INTERVAL = 1, /**< RAPs are written at a constant interval */
  IIS_XHEAACENC_RAP_OCCURRENCE_ON_DEMAND = 2          /**< RAPs are written on demand */
} IIS_XHEAACENC_RAP_OCCURRENCE;

/**********************************************************************/ /**
 The encoder state defines in which state the xHE-AAC encoder is currently.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_ENCODER_STATE_STARTUP,       /**< Encoder is in startup-phase and has not produced output yet */
  IIS_XHEAACENC_ENCODER_STATE_ENCODING,      /**< Encoder is running and producing output */
  IIS_XHEAACENC_ENCODER_STATE_WAITING,       /**< Encoder is waiting for required audio samples and does not produce output */
  IIS_XHEAACENC_ENCODER_STATE_FLUSHING,      /**< Encoder is flushing */
  IIS_XHEAACENC_ENCODER_STATE_READY_TO_CLOSE /**< Encoder is ready to be closed as all samples have been flushed */
} IIS_XHEAACENC_ENCODER_STATE;

/**********************************************************************/ /**
 The flushing mode defines how the encoder should behave after encoding the
 regular input wave files.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_FLUSHINGMODE_INVALID = -1, /**< Invalid flushing mode */
  IIS_XHEAACENC_FLUSHINGMODE_DEFAULT = 0,  /**< Flushing after encoding only as much as needed */
  IIS_XHEAACENC_FLUSHINGMODE_SYNC = 1      /**< Sync number of additional flushed samples after encoding for all AOTs (AOT 5 and 29 has by default additional samples flushed because the decoder handles SBR delay) */
} IIS_XHEAACENC_FLUSHINGMODE;

/**********************************************************************/ /**
 Data to be submitted on every call prior to IIS_xHEAACEnc_EncodeFrame()
 by means of the function IIS_xHEAACEnc_SubmitData()
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_DATA_INVALID = -1,                                /**< Invalid */
  IIS_XHEAACENC_DATA_RAP_IN_X_SAMPLES = IIS_XHEAACENC_DATA_FIRST, /**< (IIS_XHEAACENC_PARAM_INT) Trigger a RAP in x samples (x must be multiple of framelength). The value is to be supplied in number of output samples at output sampling rate. */
  IIS_XHEAACENC_DATA_LAST = IIS_XHEAACENC_PARAMETER_FIRST - 1
} IIS_XHEAACENC_DATA;

/**********************************************************************/ /**
 When adding a parameter to the parameter list each variable requires a parameter format.
 **************************************************************************/
typedef enum {
  IIS_XHEAACENC_PARAM_INVALID = -1,      /**< Invalid parameter format */
  IIS_XHEAACENC_PARAM_CHAR = 1,          /**< 8-bit character */
  IIS_XHEAACENC_PARAM_CHAR_ARRAY = 2,    /**< 8-bit character array */
  IIS_XHEAACENC_PARAM_SHORT = 3,         /**< Short integer */
  IIS_XHEAACENC_PARAM_SHORT_ARRAY = 4,   /**< Short integer array */
  IIS_XHEAACENC_PARAM_INT = 5,           /**< Integer */
  IIS_XHEAACENC_PARAM_INT_ARRAY = 6,     /**< Integer array */
  IIS_XHEAACENC_PARAM_FLOAT = 7,         /**< Floating point value */
  IIS_XHEAACENC_PARAM_FLOAT_ARRAY = 8,   /**< Floating point array */
  IIS_XHEAACENC_PARAM_DOUBLE = 9,        /**< Double precision floating point value */
  IIS_XHEAACENC_PARAM_DOUBLE_ARRAY = 10, /**< Double precision floating point array */
  IIS_XHEAACENC_PARAM_VOID_POINTER = 11, /**< Void pointer value */
  IIS_XHEAACENC_PARAM_AUTO = 96          /**< Auto detect the parameter input format */
} IIS_XHEAACENC_PARAM_FORMAT;

/**********************************************************************/ /**
 The full list of available parameters for a given encoder instance.
 **************************************************************************/
typedef enum {
  /** Invalid parameter
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: none
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_INVALID = -1,

  /** Audio Object Type
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: Defined in IIS_XHEAACENC_AOT
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_AOT = IIS_XHEAACENC_PARAMETER_FIRST,

  /** Bitrate Mode
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: Defined in IIS_XHEAACENC_BITRATEMODE
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_BITRATEMODE = 20,

  /** Bit rate
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (bps - integer multiple of 2000)
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_BITRATE = 30,

  /** Input Sample rate
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (Hz)
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_INSAMPLERATE = 60,

  /** Out Sample rate after decoding
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: Defined in IIS_XHEAACENC_OUTSAMPLERATE
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_OUTSAMPLERATE = 70,

  /** Number of channels
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: Defined in IIS_XHEAACENC_CHANNELCONFIG
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_CHANNELCONFIG = 80,

  /** Transportformat
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: Defined in IIS_XHEAACENC_TRANSPORTFORMAT
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_TRANSPORTFORMAT = 100,

  /** Frequency of the occurrence of a random access points.
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: Defined in IIS_XHEAACENC_RAP_OCCURRENCE
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_RAP_OCCURRENCE = 110,

  /** Periodic interval in milliseconds at which configuration
   * information is inserted into the stream for random access. These
   * are either immediate playout frames (IPFs), or for LATM / LOAS
   * transport this indicates the approximate interval in milliseconds
   * at which a StreamMuxConfig() is being written.
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (milliseconds)
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_MS = 120,

  /** Periodic interval in samples at which configuration information
   * can be inserted into the stream for random access. These are
   * either immediate playout frames (IPFs), or for LATM / LOAS
   * transport this indicates the interval in samples at which a
   * StreamMuxConfig() is being written.

   * The value represents a conversion of the RAP interval in
   * Milliseconds to a RAP interval in samples.

   * - The returned value is an integer multiple of 4096 samples
   *   for switchable/seekable streams
   * - The returned value is an integer multiple of frame samples
   *   for generic/single streams
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (samples)
   *  - Access: read-only
  */
  IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_SAMPLES = 130,

  /** Minimum interval within which configuration information could be
   * inserted into the stream for random access.
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (samples)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_RAP_MIN_INTERVAL_SAMPLES = 140,

  /** StreamID in the range [0 ... 65535]
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_STREAMID = 160,

  /** Minimum required output buffer size
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (chars)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_MIN_OUTBUF_SIZE = 210,
  /** Maximum possible bit rate within the duration of one second
   *  as defined in MPEG-4 Part 12
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (bps)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_BITRATELIMIT = 230,

  /** Maximum bit reservoir level
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (bits)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_BITRESERVOIRBITS_MAX = 240,

  /** Time offset of a sample from feeding it into the encoder to
   * playing it after the decoder
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (samples)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_CODECDELAY = 250,

  /** Number of leading zero in decoded bitstream
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (samples)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_PRIMING = 260,
  /** Decoder startup delay
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (samples)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_STANDARDDELAY = 270,

  /** Number of samples in a frame
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (samples)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_FRAMESAMPLES = 280,

  /**  Number of samples needed by the encoder
   *   for the next call of IIS_xHEAACEnc_EncodeFrame() to return a
   *   valid access unit
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (samples)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_SAMPLES_NEXT = 290,

  /**  Number of samples that are not yet processed by the encoder
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (samples)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_SAMPLES_LEFT = 300,

  /** Encoder Audio Profile Level Indication as defined in
   *  ISO/IEC 14496-3 Sub-Part 1 Table 1.17
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_PROFILE_LEVEL = 310,

  /** Maximum possible bitrate for a segment of audio within a random
   *  access interval
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (bits)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_MAXBITRATEPERSEGMENT = 350,

  /** Loudness instance containing the instantaneous loudness envelope
   *  - Type: (IIS_XHEAACENC_PARAM_VOID_POINTER)
   *  - Value: pointer
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA = 427,

  /** Library name
   *  - Type: (IIS_XHEAACENC_PARAM_CHAR_ARRAY)
   *  - Value: string (maximum length - 63 characters excluding
   *           string-terminating null)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_LIB_NAME = 900,

  /**  Maximum number of samples that the encoder accepts for the
   *   next call of IIS_xHEAACEnc_EncodeFrame().
   *   This number can be bigger than the number indicated by
   *   IIS_XHEAACENC_PARAMETER_SAMPLES_NEXT
   *  - Type: (IIS_XHEAACENC_PARAM_INT)
   *  - Value: int (samples)
   *  - Access: read-only
   */
  IIS_XHEAACENC_PARAMETER_SAMPLES_MAX = 930,

  /** Live loudness leveler loudness level
   *  - Type: (IIS_XHEAACENC_PARAM_FLOAT)
   *  - Value: float (LUFS)
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_LIVE_LOUDNESS_LEVEL = 1400,

  /** Live loudness leveler mode
   *  - Type: (IIS_XHEAACENC_LIVE_LOUDNESS_MODE)
   *  - Value: Defined in IIS_XHEAACENC_LIVE_LOUDNESS_MODE
   *  - Access: read-write
   */
  IIS_XHEAACENC_PARAMETER_LIVE_MODE = 1410,

  /** last parameter indicator
   *  - Access: not to be used
   */
  IIS_XHEAACENC_PARAMETER_LAST = 10000
} IIS_XHEAACENC_PARAMETER;

typedef struct xheaacenc_instance_handle *IIS_XHEAACENC_INSTANCE_HANDLE;                   /**< The xHE-AAC encoder object handle */
typedef struct xheaacenc_config_instance_struct *IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE;     /**< The xHE-AAC configuration object handle */
typedef struct xheaacenc_loudness_instance_struct *IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE; /**< The xHE-AAC loudness measurement object handle */

/**********************************************************************/ /**
 Definiton of the Access Unit (AU) info structure
 **************************************************************************/
typedef struct xheaacenc_auinfo_struct {
  unsigned int auOffset;                     /**< Offset in bits to access unit in the bitstream */
  unsigned int auSize;                       /**< Length in bytes of the access unit */
  unsigned int auSamplesValid;               /**< Number of valid samples in the access unit */
  IIS_XHEAACENC_SYNCFRAME_TYPES isSyncFrame; /**< Flag is a sync frame flag */
} IIS_XHEAACENC_AUINFO;

/**********************************************************************/ /**
 Definiton of Audio Specific Config (ASC) structure
 **************************************************************************/
typedef struct xheaacenc_ascinfo_struct {
  unsigned int ascSizeBits;                            /**< Size in bits of each ASC in output stream */
  unsigned char ascBuffer[IIS_XHEAACENC_MAX_ASC_SIZE]; /**< Character buffer containing the ACS of output stream */
} IIS_XHEAACENC_ASCINFO;

/**********************************************************************/ /**
 Definition of loudness measurement setup structure
 **************************************************************************/
typedef struct {
  unsigned int sampleRate;                   /**< Sampling rate of input audio material in Hz */
  IIS_XHEAACENC_CHANNELCONFIG channelConfig; /**< Desired channel configuration */
  uint64_t audioInputLengthSamples;          /**< Length of the audio input in samples per channel */
  unsigned int audioInputLengthAvailable;    /**< Flag that indicates whether the length of the audio input is specified with "audioInputLengthSamples" */
} IIS_XHEAACENC_LOUDNESS_SETUP;

/**********************************************************************/ /**
 Typedef for a message callback function which is used to print informational status messages.
 **************************************************************************/
typedef void (*IIS_XHEAACENC_MESSAGE_CALLBACK)(char *message);

/**********************************************************************/ /**
 Set a user defined message callback function.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_SetMessageCallback(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig,        /**< inout: Pointer to config handle */
    IIS_XHEAACENC_MESSAGE_CALLBACK const messageCallback /**< in: Pointer to message function */
);

/**********************************************************************/ /**
 Create a list with the minimum number of parameters required to configure an encoder instance.
 The function input parameters are the minimum set required to create the
 parameter list. More parameters can be added via IIS_xHEAACEnc_Config_AddParamValueXXX().
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_Open(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE *phConfig, /**< out: Pointer to handle the configurations */
    int const inSampleRate,                         /**< in: Sampling rate in of input material in Hz */
    IIS_XHEAACENC_CHANNELCONFIG const channelConfig /**< in: Desired channel configuration */
);

/**********************************************************************/ /**
 Add an integer type parameter to the parameter list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_AddParamValueInt(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig, /**< inout: Handle to parameter list */
    IIS_XHEAACENC_PARAMETER const paramTag,       /**< in: Parameter name/tag */
    int const paramValue                          /**< in: Parameter value */
);

/**********************************************************************/ /**
 Add a float type parameter to the parameter list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_AddParamValueFloat(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig, /**< inout: Handle to parameter list */
    IIS_XHEAACENC_PARAMETER const paramTag,       /**< in: Parameter name */
    float const paramValue                        /**< in: Parameter value */
);

/**********************************************************************/ /**
 Add a void pointer type parameter to the parameter list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_AddParamValuePointer(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig, /**< inout: Handle to parameter list */
    IIS_XHEAACENC_PARAMETER const paramTag,       /**< in: Parameter name */
    void const *const paramValue                  /**< in: Void pointer */
);
/**********************************************************************/ /**
 Add an array type parameter to the parameter list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_AddParamArray(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig, /**< inout: Handle to parameter list */
    IIS_XHEAACENC_PARAMETER const paramTag,       /**< in: Parameter name */
    IIS_XHEAACENC_PARAM_FORMAT const paramFormat, /**< in: Parameter format i.e. char, int */
    void const *const pParamValue,                /**< in: Pointer to parameter value(s) */
    int const paramLength                         /**< in: Number of values */
);

/**********************************************************************/ /**
 Remove a parameter from the parameter list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_DeleteParam(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig, /**< inout: Handle to parameter list */
    IIS_XHEAACENC_PARAMETER const param           /**< in: Parameter tag */
);

/**********************************************************************/ /**
 Validate the configuration. This function can be used to validate the configuration
 without a loudness level, e.g. before the loudness measurement.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function and that the config is valid. A return code of IIS_XHEAACENC_WARNING_INVALID_CONFIG
 indicates that the config in the current state is not valid. Any other return code indicates
 an error. If the config is invalid, the parameter validateErrorInfo will provide more information
 on the reason. See the 'Error Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_Validate(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, /**< in: Config instance handle */
    IIS_XHEAACENC_RETURN_CODE *const validateErrorInfo  /**< out: Error information if config is invalid */
);

/**********************************************************************/ /**
 Finalize and validate the configuration.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_Finalize(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig /**< in: Config instance handle */
);

/**********************************************************************/ /**
 Close a configuration instance.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_Delete(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig /**< in: Config instance handle */
);

/**********************************************************************/ /**
 This function opens and initializes one loudness instance used by a subsequent
 loudness measurement pass. The initialization is dependent on the parameter
 in the loudnessSetup struct.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other value indicates an error. See the 'Error Handling' section
 for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Open(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE *const phLoudness, /**< out: Pointer to a loudness instance handle */
    IIS_XHEAACENC_LOUDNESS_SETUP const loudnessSetup          /**< in: Struct that contains parameters for the loudness measurement initialization */
);

/**********************************************************************/ /**
 Measure integrated loudness, loudness envelope and sample peak.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other value indicates an error. See the 'Error Handling' section
 for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Measure(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness, /**< inout: Loudness instance handle */
    float const *const pSamples,                            /**< in: Pointer to the uncompressed interleaved input audio samples. The samples
                                                                     must be of type float, the sample values must be within the range [-1.0 ... 1.0] */
    int const nSamples                                      /**< in: The number of valid input samples. This value must to be a multiple of number of input channels */
);

/**********************************************************************/ /**
 Closes a loudness instance.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other value indicates an error. See the 'Error Handling' section
 for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Delete(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE hLoudness /**< in: Loudness instance handle */
);

/**********************************************************************/ /**
 This function exports loudness instance data generated by a loudness measurement pass.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other value indicates an error. See the 'Error Handling' section
 for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Export(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness, /**< in: Loudness instance handle */
    unsigned char const **const loudnessData,               /**< out: The loudness data as byte array */
    unsigned int *const numBytes                            /**< out: The number of bytes the array loudnessData contains */
);

/**********************************************************************/ /**
 This function imports loudness instance data from a file.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other value indicates an error. See the 'Error Handling' section
 for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Import(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness, /**< inout: Loudness instance handle */
    unsigned char const *const loudnessData,                /**< in: Loudness data as byte array */
    unsigned int const numBytes                             /**< in: Number of bytes the array loudnessData contains */
);

/**********************************************************************/ /**
 This function opens and initializes one instance of the encoder.
 The encoder configuration is dependent on the parameter list provided.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Open(
    IIS_XHEAACENC_INSTANCE_HANDLE *phxHEAACEnc,        /**< out: Pointer to an encoder handle */
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig /**< in:  Configuration */
);

/**********************************************************************/ /**
 Delete the encoder instance.
 Call this function to de-initialize and close the encoder instance. All memory
 and other allocated resources will be freed.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Delete(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc /**< inout: Pointer to encoder instance handle */
);

/**********************************************************************/ /**
 Main encoding function. This function converts one frame of uncompressed
 audio data into an ISO/MPEG Extended HE-AAC bit stream payload.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_EncodeFrame(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc, /**< inout: A valid, pre-configured encoder handle */
    float const *const pSamples,              /**< in: Pointer to the uncompressed interleaved input audio samples. The samples
                                                       must be of type float, the sample values must be within the range [-1.0 ... 1.0] */
    const int nSamples,                       /**< in: The number of valid input samples. This value must to be a multiple of number of input channels */
    unsigned char *const pOutput,             /**< inout: A pointer to a user supplied buffer to hold the output. Only complete frames are returned */
    int *const pOutputBytes,                  /**< out: Upon return, this value will hold the number of valid bytes in the output buffer */
    const unsigned int outputBufSizeBytes,    /**< in: Size of output buffer in bytes */
    IIS_XHEAACENC_AUINFO *const pAuInfo       /**< out: Pointer to structure which contains information about access unit */
);

/**********************************************************************/ /**
 Submit frame by frame data of type IIS_XHEAACENC_DATA prior to a call of IIS_xHEAACEnc_EncodeFrame().
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_SubmitData(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc,    /**< inout: A valid, pre-configured encoder handle */
    IIS_XHEAACENC_DATA const type,               /**< in: Type of submitted data */
    IIS_XHEAACENC_PARAM_FORMAT const dataFormat, /**< in: Parameter format i.e. char, int */
    void const *const pData,                     /**< in: Pointer to buffer from which data will be copied */
    int const dataSize                           /**< in: Size of data to submit in chars */
);

/**********************************************************************/ /**
 Get information about specific parameters from the encoder.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetParam(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc,     /**< in: A valid, pre-configured encoder handle */
    IIS_XHEAACENC_PARAMETER const paramTag,       /**< in: Parameter name/tag */
    IIS_XHEAACENC_PARAM_FORMAT const paramFormat, /**< in: Parameter format ,i.e. int, float, double, automatic */
    void *const pData,                            /**< out: Pointer to buffer into which data will be copied */
    int const size                                /**< in: Size of data to get in chars */
);

/**********************************************************************/ /**
 Get Audio specific config information.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetAscInfo(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc, /**< in: A valid, pre-configured encoder handle */
    IIS_XHEAACENC_ASCINFO *const pAscInfo     /**< out: Buffer containing ASC information */
);

/**********************************************************************/ /**
 Get Encoder State.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetEncoderState(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc,       /**< in:  Encoder handle */
    IIS_XHEAACENC_ENCODER_STATE *const encoderState /**< out: Encoder state */
);

/**********************************************************************/ /**
 Collects all the warnings that have been raised by the instance since the
 last call to this function. The warning array resets after every call to
 this function.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetEncodingWarnings(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc,      /**< inout: Pointer to instance handle */
    int *const pnWarning,                          /**< out: Pointer to number of warnings */
    IIS_XHEAACENC_WARNING const **const ppWarnings /**< out: Pointer to array of warnings */
);

/**********************************************************************/ /**
 Collects all the warnings that have been raised during configuration since the
 last call to this function. The warning array resets after every call to
 this function.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetConfigWarnings(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, /**< inout: Pointer to instance handle */
    int *const pnWarning,                               /**< out: Number of warnings */
    IIS_XHEAACENC_WARNING const **const ppWarnings      /**< out: Pointer to array of warnings */
);

/**********************************************************************/ /**
 Gets the version information for this library.
 \returns version info as const string
 **************************************************************************/
char const *IIS_XHEAACAPI IIS_xHEAACEnc_GetVersionInfoStr(void);

#if defined(WIN32)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif
